// dice_device_enrollment.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "DiceCore.h"
#include "DiceRIoT.h"

// User inputs
const char *udsString = "19e25a259d0c2be03a02d416c05c48ccd0cc7d1743458aae1cb488b074993eae"; //This is sample UDS string
const char *macAddress = "[Devkit_Mac_Address]";
char firmwareVer[10] = "[version]";
const char *binFileFullPath = "C:\\Users\\[alias]\\AppData\\Local\\Arduino15\\packages\\AZ3166\\hardware\\stm32f4\\1.1.0\\libraries\\AzureIoT\\examples\\DPS\\.build\\DPS.ino.bin";
const char *mapFileFullPath = "C:\\Users\\[alias]\\AppData\\Local\\Arduino15\\packages\\AZ3166\\hardware\\stm32f4\\1.1.0\\libraries\\AzureIoT\\examples\\DPS\\.build\\DPS.ino.map";

// Settings
uint32_t udsStringLength = 64;
uint32_t macAddressLength = 12;
uint32_t elementSize = 1; //bytes
uint32_t aliasCertSize = 1024; //bytes

// Flash start address name
const char * flashStartname = "FLASH            0x";

// Riot attribute names
const char * startRiotCoreName = "__start_riot_core = .";
const char * stopRiotCoreName = "__stop_riot_core = .";
const char * startRiotFwName = "__start_riot_fw = .";
const char * stopRiotFwName = "__stop_riot_fw = .";

// Flash start info
uint8_t * startBin;

// Riot Core Info
uint8_t * startRiotCore;
uint8_t * endRiotCore;

// Riot Firmware Info
uint8_t * startRiotFw;
uint8_t * endRiotFw;

// UDS bytes for DICE|RIoT calculation
uint8_t udsBytes[DICE_UDS_LENGTH] = { 0 };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validate user input data sanity
static int udsStringValidated()
{
	// Check length
	int length = strlen(udsString);
	if (length != udsStringLength)
	{
		printf("udsString must be %d in length but your input length is %d.\r\n", udsStringLength, length);
		return 1;
	}

	// Check each character
	int tempRes = 0;
	char element[2];
	memset(element, 0, 2);
	int badCharCount = 0;
	for (int i = 0; i < udsStringLength; i++)
	{
		element[0] = udsString[i];
		tempRes = strtoul(element, NULL, 16);
		if (tempRes == 0 && element[0] != '0')
		{
			badCharCount++;
			printf("udsString can only contain characters in Hex value (between 0 and e) and the %dth character in your string is %s.\r\n", i + 1, element);
		}
	}

	if (badCharCount > 0)
	{
		return 1;
	}
	return 0;
}

static int macAddressValidated()
{
	// Check length
	int length = strlen(macAddress);
	if (length != macAddressLength)
	{
		printf("macAddress must be %d in length but your input length is %d.\r\n", macAddressLength, length);
		return 1;
	}
	return 0;
}

static int firmwareVerValidated()
{
	// Check format
	int dotCount = 0;

	int tempRes = 0;
	char element[2];
	memset(element, 0, 2);
	int badCharCount = 0;

	for (int i = 0; i < strlen(firmwareVer); i++) 
	{
		element[0] = firmwareVer[i];
		if (element[0] == '.') {
			dotCount ++;
		}
		else
		{
			tempRes = strtoul(element, NULL, 10);
			if (tempRes == 0 && element[0] != '0')
			{
				badCharCount++;
				printf("firmwareVer must contain characters between dots but the %dth character in your input is %s.\r\n", i + 1, element);
			}
		}
	}
	if (dotCount != 2)
	{
		printf("firmwareVer must be num.num.num in format but your input is in wrong format.\r\n");
	}
	if (badCharCount > 0)
	{
		return 1;
	}

	return 0;
}

static int fileFullPathValidated(const char * fileFullPath)
{
	FILE *fp;
	fp = fopen(fileFullPath, "rb"); // open the file in binary format
	if (fp == NULL) {
		printf("Unable to open file %s.\r\n", fileFullPath);
		return 1;
	}

	fclose(fp);
	return 0;
}

static int validateUserInputData()
{
	if (udsStringValidated() || macAddressValidated() || firmwareVerValidated() || fileFullPathValidated(binFileFullPath) || fileFullPathValidated(mapFileFullPath))
	{
		return 1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Prepare DICE|RIOT data
static int getRegistrationId(char * registrationId)
{
	for (int i = 0; i < strlen(firmwareVer); i++) {
		if (firmwareVer[i] == '.') {
			firmwareVer[i] = 'v';
		}
	}
	return sprintf(registrationId, "az-%sv%s", macAddress, firmwareVer);
}

static int getUDSBytesFromString()
{
	char element[2];
	unsigned int resLeft;
	unsigned int resRight;

	memset(element, 0, 2);
	for (int i = 0; i < 32; i++) {
		element[0] = udsString[i * 2];
		resLeft = strtoul(element, NULL, 16);
		element[0] = udsString[i * 2 + 1];
		resRight = strtoul(element, NULL, 16);
		udsBytes[i] = (resLeft << 4) + resRight;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Read machine code from binary file
static int getDataFromBinFile(uint8_t * startAddress, uint8_t * buffer, uint32_t size)
{
	int result = 0;
	// Open binary file
	FILE *fp;
	fp = fopen(binFileFullPath, "rb"); // open the file in binary format
	if (fp == NULL) {
		printf("Unable to open file\r\n");
		result = 1;
		return result;
	}

	long int offset = startAddress - startBin;
	result = fseek(fp, offset, SEEK_SET);
	if (result != 0)
	{
		printf("Failed to seek offset %d for binary file %s.\r\n", offset, binFileFullPath);
		result = 1;
		return result;
	}

	uint8_t * curser = buffer;
	for (int i = 0; i < size; i++) {
		result = fread(curser, elementSize, 1, fp);
		if (result != elementSize)
		{
			printf("Failed to read data in size %d bytes from binary file %s.\r\n", elementSize, binFileFullPath);
			result = 1;
			return result;
		}
		curser += elementSize;
	}

	// Close binary file
	fclose(fp);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Read RIOT attribute data from map file
static unsigned long int findAddressInMapFile(const char * attributeName)
{
	unsigned long int result = 0;
	FILE *fpm;
	char str[64];

	/* opening file for reading */
	fpm = fopen(mapFileFullPath, "r");
	if (fpm == NULL) {
		perror("Error opening file");
		return(-1);
	}

	while ((fgets(str, 64, fpm)) != NULL) {
		if ((strstr(str, attributeName)) != NULL) {
#if logging
			printf("A match found on line: %s\r\n", str);
#endif
			int pos = 0;
			for (int i = 0; i < 64; i++) {
				if (str[i] == 'x') {
					pos = i + 1;
					break;
				}
			}

			unsigned long int tempInt = 0;
			char temp[2];
			memset(temp, 0, 2);
			for (int i = 0; i < 8; i++) {
				temp[0] = str[pos + i];
				tempInt = strtoul(temp, NULL, 16);
				result = result + (tempInt << ((8 - i - 1) * 4));
			}
#if logging
			printf("%x", result);
#endif
		}
	}

	fclose(fpm);

	if (result == 0)
	{
		printf("No matching for keyword %s is found in file %s.\r\n", attributeName, mapFileFullPath);
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// App start
int main()
{
	// Check sanity of input data
	if (validateUserInputData() != 0)
	{
		(void)printf("Press any key to continue:\r\n");
		(void)getchar();
		return 1;
	}

	// Prepare UDS from udsString
	getUDSBytesFromString();

	// Get desired firmware version and device's registration Id
	char registrationId[32] = {'\0'};
	getRegistrationId(registrationId);

	// Get start address of flash from .bin file
	unsigned long int result;
	result = findAddressInMapFile(flashStartname);
	if (result == 0)
	{
		(void)printf("Press any key to continue:\r\n");
		(void)getchar();
		return 1;
	}
	startBin = (uint8_t*)result;

	// Initilize the value of riot attributes stuff
	result = findAddressInMapFile(startRiotCoreName);
	if (result == 0)
	{
		(void)printf("Press any key to continue:\r\n");
		(void)getchar();
		return 1;
	}
	startRiotCore = (uint8_t*)result;

	result = findAddressInMapFile(stopRiotCoreName);
	if (result == 0)
	{
		(void)printf("Press any key to continue:\r\n");
		(void)getchar();
		return 1;
	}
	endRiotCore = (uint8_t*)result;

	result = findAddressInMapFile(startRiotFwName);
	if (result == 0)
	{
		(void)printf("Press any key to continue:\r\n");
		(void)getchar();
		return 1;
	}
	startRiotFw = (uint8_t*)result;

	result = findAddressInMapFile(stopRiotFwName);
	if (result == 0)
	{
		(void)printf("Press any key to continue:\r\n");
		(void)getchar();
		return 1;
	}
	endRiotFw = (uint8_t*)result;

	// Read Riot_core machine code from .bin file
	uint32_t riotSize = endRiotCore - startRiotCore;
	uint8_t *riotCore = (uint8_t*)malloc(riotSize);
	memset(riotCore, 0, riotSize);

	if (getDataFromBinFile(startRiotCore, riotCore, riotSize) != 0) {
		free(riotCore);
		(void)printf("Press any key to continue:\r\n");
		(void)getchar();
		return 1;
	}

	// Read Riot_fw machine code from .bin file
	uint32_t riotFwSize = endRiotFw - startRiotFw;
	uint8_t * riotFw = (uint8_t*)malloc(riotFwSize);
	memset(riotFw, 0, riotFwSize);

	if (getDataFromBinFile(startRiotFw, riotFw, riotFwSize) != 0) {
		free(riotFw);
		(void)printf("Press any key to continue:\r\n");
		(void)getchar();
		return 1;
	}

	// Retrieve alias certificate
	char* aliasCertBuffer = (char*)malloc(aliasCertSize);
	memset(aliasCertBuffer, 0, aliasCertSize);
	DiceRIoTStart(registrationId, riotCore, riotSize, riotFw, riotFwSize, aliasCertBuffer, aliasCertSize);

	// Write the cert to pem file
	FILE * opening;
	char certFileName[64] = {'\0'};
	sprintf(certFileName, "%s.pem", registrationId);
	opening = fopen(certFileName, "w");
	fprintf(opening, aliasCertBuffer);
	fclose(opening);
	printf("Writing to the Alias Key Certificate file %s is successful.\n", certFileName);

	// Release allocations in heap
	free(riotCore);
	free(riotFw);
	free(aliasCertBuffer);

	(void)printf("Press any key to continue:\r\n");
	(void)getchar();
    return 0;
}

