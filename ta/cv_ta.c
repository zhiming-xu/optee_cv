#include <inttypes.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include "cv_ta.h"
#include "sod.h"

struct aes_cipher {
	uint32_t algo;			/* AES flavour */
	uint32_t mode;			/* Encode or decode */
	uint32_t key_size;		/* AES key size in byte */
	TEE_OperationHandle op_handle;	/* AES ciphering operation */
	TEE_ObjectHandle key_handle;	/* transient object to load the key */
};

#define AES128_KEY_BIT_SIZE		128
#define AES128_KEY_BYTE_SIZE		(AES128_KEY_BIT_SIZE / 8)
#define AES256_KEY_BIT_SIZE		256
#define AES256_KEY_BYTE_SIZE		(AES256_KEY_BIT_SIZE / 8)
#define AES_BLOCK_SIZE          16

const static char key[AES256_KEY_BYTE_SIZE] = "$B&E)H+MbQeThWmZq4t7w!z%C*F-JaNc";
const static char iv[AES_BLOCK_SIZE] = "an iv here";

static int filter_cb(int w, int h) {
	/* discard regions of non interest */
	if ((w > 300 && h > 200)  || w < 45 || h < 45) {
		return 0;
	}
	return 1;
}

static void *process_image(unsigned char* buffer, uint32_t input_size, uint32_t *output_size) {
    sod_img img = sod_img_load_from_mem(buffer, input_size, 1);
    if (img.data == 0) {
        EMSG("Cannot load input image");
        return NULL;
    }
    sod_img threshold_img = sod_threshold_image(img, 0.5);
    sod_free_image(img);
    sod_img canny_img = sod_canny_edge_image(threshold_img, 1);
    sod_free_image(threshold_img);
    sod_img dilate_img = sod_dilate_image(canny_img, 12);
    sod_free_image(canny_img);
    
    sod_box *box = NULL;
    int nbox;
    sod_image_find_blobs(dilate_img, &box, &nbox, filter_cb);
    DMSG("nbox: %d", nbox);
    sod_free_image(dilate_img);
    sod_img color_img = sod_img_load_from_mem(buffer, input_size, 3);
    for (int i = 0; i < nbox; i++) {
        DMSG("x: %d, y: %d, w: %d, h: %d", box[i].x, box[i].y, box[i].w, box[i].h);
        sod_image_draw_bbox_width(color_img, box[i], 3, 0., 0., 255.);
    }
    sod_image_blob_boxes_release(box);
    DMSG("Finish object detection");

    unsigned char *png = sod_img_dump_as_png(color_img, output_size);
    sod_free_image(color_img);
    
    buffer = TEE_Realloc(buffer, *output_size);
    if (buffer == NULL) {
        EMSG("out of memory");
        return NULL;
    }
    TEE_MemMove(buffer, png, *output_size);
    TEE_Free(png);
    
    DMSG("Finish saving to char, size: %d", *output_size);
    return buffer;
}

static TEE_Result prepare_operation(struct aes_cipher *sess) {
    TEE_Result res;
    /* Free potential previous transient object */
    if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);

	/* Allocate operation: AES/CTR, mode and size from params */
	IMSG("Allocate sess->op_handle");
    res = TEE_AllocateOperation(&sess->op_handle,
				    sess->algo,
				    sess->mode,
				    sess->key_size * 8);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate operation");
		sess->op_handle = TEE_HANDLE_NULL;
		goto err;
	}

	/* Free potential previous transient object */
	if (sess->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(sess->key_handle);

	IMSG("Allocate sess->key_handle");
	/* Allocate transient object according to target key size */
	res = TEE_AllocateTransientObject(TEE_TYPE_AES,
					  sess->key_size * 8,
					  &sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate transient object");
		sess->key_handle = TEE_HANDLE_NULL;
		goto err;
	}

	IMSG("Set sess->key_handle");
    TEE_Attribute attr;
	TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, key, sess->key_size);

	res = TEE_PopulateTransientObject(sess->key_handle, &attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed, %x", res);
        goto err;
	}

	IMSG("Set sess->op_handle");
	res = TEE_SetOperationKey(sess->op_handle, sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_SetOperationKey failed %x", res);
        goto err;
	}

    err:  
        return res;
}

static TEE_Result cmd_exec(uint32_t types,
			     TEE_Param params[TEE_NUM_PARAMS])
{
	TEE_Result rc = TEE_ERROR_GENERIC;
	const int input_idx = 0;		/* highlight nonsecure buffer index */
    const int output_idx = 1;

	if (types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				                 TEE_PARAM_TYPE_MEMREF_OUTPUT,
				                 TEE_PARAM_TYPE_NONE,
				                 TEE_PARAM_TYPE_NONE)) {
		EMSG("bad parameters %x", (unsigned)types);
		return TEE_ERROR_BAD_PARAMETERS;
	}

    uint32_t input_size = params[input_idx].memref.size;
    void *tee_buffer = TEE_Malloc(input_size, TEE_MALLOC_FILL_ZERO);
	if (tee_buffer == NULL)
		return TEE_ERROR_SHORT_BUFFER;

	/*
	 * We could rely on the TEE to provide consistent buffer/size values
	 * to reference a buffer with a unique and consistent secure attribute
	 * value. Hence it is safe enough (and more optimal) to test only the
	 * secure attribute of a single byte of it. Yet, since the current
	 * test does not deal with performance, let check the secure attribute
	 * of each byte of the buffer.
	 */
	rc = TEE_CheckMemoryAccessRights(TEE_MEMORY_ACCESS_ANY_OWNER |
					 TEE_MEMORY_ACCESS_READ |
					 TEE_MEMORY_ACCESS_NONSECURE,
					 params[input_idx].memref.buffer,
					 params[input_idx].memref.size);
	if (rc != TEE_SUCCESS) {
		EMSG("TEE_CheckMemoryAccessRights (nsec) failed %x", rc);
		return rc;
	}

    // ---------- decrypt data --------
    // create cipher object
    struct aes_cipher cipher = {
        TEE_ALG_AES_CTR,
        TEE_MODE_DECRYPT,
        AES256_KEY_BYTE_SIZE,
        TEE_HANDLE_NULL,
        TEE_HANDLE_NULL
    };

    struct aes_cipher *sess = &cipher;

    // prepare cipher object
    rc = prepare_operation(sess);
    
    if (rc != TEE_SUCCESS) {
        EMSG("Prepare decryption session failed %x", rc);
        goto err;
    }
    
    // initialize cipher state
    TEE_CipherInit(sess->op_handle, (void*)iv, AES_BLOCK_SIZE);

    // do cipher
    rc = TEE_CipherUpdate(sess->op_handle,
				          params[input_idx].memref.buffer, params[input_idx].memref.size,
				          tee_buffer, &input_size);

    uint32_t output_size = input_size;
    tee_buffer = process_image(tee_buffer, input_size, &output_size);
    DMSG("------- in: %d, out: %d --------", input_size, output_size);
 
    // --------- encrypt data ---------
    sess->mode = TEE_MODE_ENCRYPT;
    rc = prepare_operation(sess);
    
    if (rc != TEE_SUCCESS) {
        EMSG("Prepare decryption session failed %x", rc);
        goto err;
    }

    TEE_CipherInit(sess->op_handle, (void*)iv, AES_BLOCK_SIZE);
    
    rc = TEE_CipherUpdate(sess->op_handle, tee_buffer, output_size,
				          params[output_idx].memref.buffer, &params[output_idx].memref.size);

    if (rc != TEE_SUCCESS) {
        EMSG("Doing cipher failed %x", rc);
        goto err;
    }
	
    err:
        if (sess->op_handle != TEE_HANDLE_NULL)
	     	TEE_FreeOperation(sess->op_handle);
	    sess->op_handle = TEE_HANDLE_NULL;

	    if (sess->key_handle != TEE_HANDLE_NULL)
	    	TEE_FreeTransientObject(sess->key_handle);
        sess->key_handle = TEE_HANDLE_NULL;

    TEE_Free(tee_buffer);
	return rc;
}

TEE_Result TA_CreateEntryPoint(void)
{
	/* Nothing to do */
    IMSG("Create TA\n");
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	/* Nothing to do */
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types,
					TEE_Param __unused params[4],
					void __unused **session)
{
	// *session = (void *)sess;
	DMSG("Entry");
    return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __unused *session)
{
    DMSG("Exit");
}

TEE_Result TA_InvokeCommandEntryPoint(void __unused *session,
					uint32_t cmd,
					uint32_t param_types,
					TEE_Param params[4])
{
	switch (cmd) {
	case TA_CV_CMD_EXEC:
		return cmd_exec(param_types, params);
	default:
		EMSG("Command ID 0x%x is not supported", cmd);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}