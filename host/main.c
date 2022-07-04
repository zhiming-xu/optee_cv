#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <cv_ta.h>

#define OUTPUT_BUFFER_SIZE 512 * 1024 // at most 512KB


void* read_file(char *filename, uint32_t *size) {
    FILE *f = fopen(filename, "rb");
    void* buffer = NULL;
    if (f) {
        fseek(f, 0, SEEK_END);
        *size = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(*size);
        if (buffer) {
            fread((unsigned char*)buffer, 1, *size, f);
        }
        fclose(f);
    }
    return buffer;
}

int write_file(void *buffer, char *filename, size_t size) {
    FILE *f = fopen(filename, "wb");
    size_t bytes = fwrite((unsigned char*)buffer, 1, size, f);
    fclose(f);
    return bytes;
}

int main(int argc, char *argv[]) {
    int total_imgs = atoi(argv[1]);
    char *input_file = malloc(20);
    char *output_file = malloc(20);

    TEEC_Context ctx;
    TEEC_Session sess;
    
    void *output = malloc(OUTPUT_BUFFER_SIZE); // in case the size of image increases
    if (output == NULL) {
        err(1, "Not enough memory\n");
    }

    TEEC_UUID uuid = TA_CV_UUID;
	uint32_t origin;
	TEEC_Result res;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/* Open a session with the TA */
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			 res, origin);

    int cnt = 0;
    double start_tick = clock();
    for (int i = 0; ; i++) {
        sprintf(input_file, "Cars%d_crop_sec", i);
        sprintf(output_file, "image_out_%d", i);

        uint32_t secret_size = 0;
        void *secret_content = read_file(input_file, &secret_size);
        if (secret_content == NULL) {
            fprintf(stderr, "\033[31mNo content read from file: %s\033[0m\n", input_file);
            goto contd;
        }
 
        TEEC_Operation op;
	    memset(&op, 0, sizeof(op));
	    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
	    				                 TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);
	    op.params[0].tmpref.buffer = (void*)secret_content;
	    op.params[0].tmpref.size = secret_size;
        op.params[1].tmpref.buffer = output;
        op.params[1].tmpref.size = OUTPUT_BUFFER_SIZE;

	    res = TEEC_InvokeCommand(&sess, TA_CV_CMD_EXEC, &op, &origin);
	    if (res != TEEC_SUCCESS) {
	    	errx(1, "\033[31mTEEC_InvokeCommand (CV) failed 0x%x origin 0x%x for %s\033[0m",
	    	     res, origin, output_file);
        }
        size_t bytes = write_file(op.params[1].tmpref.buffer, output_file, op.params[1].tmpref.size);
        cnt++;
        // printf("\033[32mSuccessfully write %d bytes to %s\033[0m\n", bytes, output_file);
        contd:
        free(secret_content); // free the input buffer
        memset(output, 0, OUTPUT_BUFFER_SIZE); // invalidate content in the output buffer
        if (cnt == total_imgs) {
            break;
        }
    } 
    double end_tick = clock();
    double time_ms = (end_tick - start_tick) * 1000 / CLOCKS_PER_SEC;

    printf("throughtput: %.2f img/s on %d samples\n", cnt / (time_ms / 1000), cnt);
    free(output);
    TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
    return 0;
}