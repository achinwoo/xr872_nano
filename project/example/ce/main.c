/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include "kernel/os/os.h"
#include "driver/chip/hal_crypto.h"

char *aes_key = "1234567812345678";

static uint32_t aes_enc(char *src_data, char *enc_buf)
{
	HAL_Status status = HAL_ERROR;

	CE_AES_Config aes_cfg;
	memset(&aes_cfg, 0, sizeof(aes_cfg));

	sprintf((char*)aes_cfg.key, aes_key);
	aes_cfg.keysize = CE_CTL_AES_KEYSIZE_128BITS;
	aes_cfg.mode = CE_CRYPT_MODE_CBC;
	aes_cfg.src = CE_CTL_KEYSOURCE_INPUT;

	status = HAL_AES_Encrypt(&aes_cfg, (uint8_t*)src_data, (uint8_t*)enc_buf, strlen(src_data));
	if (status != HAL_OK) {
		printf("AES Encrypt error %d\r\n", status);
		return 0;
	}

	uint32_t dec_data_len = 0;
	dec_data_len = (strlen(src_data) + 15) & (~0xF);

	return dec_data_len;
}

static void aes_decrypt(char *enc_data, uint32_t enc_data_len, char *dec_buf)
{
	HAL_Status status = HAL_ERROR;
	CE_AES_Config aes_cfg;
	memset(&aes_cfg, 0, sizeof(aes_cfg));

	sprintf((char*)aes_cfg.key, aes_key);
	aes_cfg.keysize = CE_CTL_AES_KEYSIZE_128BITS;
	aes_cfg.mode = CE_CRYPT_MODE_CBC;
	aes_cfg.src = CE_CTL_KEYSOURCE_INPUT;

	status = HAL_AES_Decrypt(&aes_cfg, (uint8_t*)enc_data, (uint8_t*)dec_buf, enc_data_len);
	if (status != HAL_OK) {
		printf("AES decrypt error %d\r\n", status);
	}
}

void aes_demo()
{
	printf("AES:\r\n");

	char *src_data = "1234567887654321";
	printf("\tsrc_data: \"%s\"\r\n", src_data);

	char enc_buf[100];
	char dec_buf[100];
	uint32_t enc_data_len;

	memset(enc_buf, 0, 100);
	enc_data_len = aes_enc(src_data, enc_buf);
	if (!enc_data_len)
		printf("aes encrypt error\r\n");

	printf("\teecrypt data len %d\r\n", enc_data_len);
	printf("\tencrypt data: %s \r\n", enc_buf);

	memset(dec_buf, 0, 100);
	aes_decrypt(enc_buf, enc_data_len, dec_buf);
	printf("\tdecrypt data : \"%s\"\r\n\r\n", dec_buf);

}

void crc_demo()
{
	printf("CRC:\r\n");

	HAL_Status status = HAL_ERROR;
	CE_CRC_Handler hdl;
	memset(&hdl, 0, sizeof(hdl));

	char *data = "123456";
	uint32_t crc_value = 0;

	printf("\tsrc data: \"%s\"\r\n", data);
	status = HAL_CRC_Init(&hdl, CE_CRC16_IBM, strlen(data));
	if (status != HAL_OK) {
		printf("CRC init error %d\r\n", status);
		return;
	}

	status = HAL_CRC_Append(&hdl, (uint8_t*)data, strlen(data));
	if (status != HAL_OK) {
		printf("CRC append error %d\r\n", status);
		return;
	}

	status = HAL_CRC_Finish(&hdl, &crc_value);
	if (status != HAL_OK) {
		printf("CRC finish error %d\r\n", status);
		return;
	}

	printf("\tresult : %d\r\n\r\n", crc_value);
}

void md5_demo()
{
	printf("MD5:\r\n");

	HAL_Status status = HAL_ERROR;
	CE_MD5_Handler hdl;
	uint32_t digest[4];
	char *data = "123456";

	printf("\tsrc data: %s\r\n", data);
	status = HAL_MD5_Init(&hdl, CE_CTL_IVMODE_SHA_MD5_FIPS180, NULL);
	if (status != HAL_OK) {
		printf("MD5 init error %d\r\n", status);
		return;
	}

	status = HAL_MD5_Append(&hdl, (uint8_t*)data, strlen(data));
	if (status != HAL_OK) {
		printf("MD5 append error %d\r\n", status);
		return;
	}

	status = HAL_MD5_Finish(&hdl, digest);
	if (status != HAL_OK) {
		printf("MD5 finish error %d\r\n", status);
		return;
	}

	printf("\tresult :\r\n");
	int i = 0;
	while(i < 4) {
		printf("\t\tdigest[%d] = 0x%x\r\n", i, digest[i]);
		i++;
	}

	printf("\r\n");
}

void random_demo()
{
	printf("RANDOM:\r\n");

	HAL_Status status = HAL_ERROR;
	uint8_t random_buf[2];
	uint32_t random = 0;

	status = HAL_PRNG_Generate(random_buf, 2);
	if (status != HAL_OK) {
		printf("prng genrate error %d\r\n", status);
		return;
	}

	random = (random_buf[0] << 8) + random_buf[1];
	printf("\tresult: %d\r\n\r\n", random);
}

int main(void)
{
	printf("ce demo started\r\n\r\n");

	HAL_Status status = HAL_ERROR;
	status = HAL_CE_Init();
	if (status != HAL_OK) {
		printf("CE init error\r\n");
		return 1;
	}

	aes_demo();
	crc_demo();
	md5_demo();
	random_demo();

	status = HAL_CE_Deinit();
	if (status != HAL_OK)
		printf("CE deinit error\r\n");

	printf("ce demo end\r\n");

	while (1) {
		OS_MSleep(10000);
	}

	return 0;
}
