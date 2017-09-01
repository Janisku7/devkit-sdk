/*(Copyright)

Microsoft Copyright 2015, 2016
Confidential Information

*/

#include "RiotCore.h"
#include <stdio.h>

// RIoT Core data
static uint8_t cDigest[RIOT_DIGEST_LENGTH];
static uint8_t FWID[RIOT_DIGEST_LENGTH];
static uint8_t derBuffer[DER_MAX_TBS];

// Keys and Certs for upper layer(s)
char g_DeviceIDPub[DER_MAX_PEM] = {0};
char g_AliasKey[DER_MAX_PEM] = {0};
char g_AKCert[DER_MAX_PEM] = {0};
char g_DICert[DER_MAX_PEM] = {0};

// Lengths
unsigned int g_DeviceIDPubLen;
unsigned int g_AliasKeyLen;
unsigned int g_AKCertLen;
unsigned int g_DICertLen;


// We don't do CSRs on this device
static bool SelfSignedDeviceCert = true;

// Location of FW image
#define FW_BASE_ADDR  Load$$ER_IROM2$$Base
#define FW_LIMIT_ADDR Load$$ER_IROM2$$Limit
extern uint8_t FW_BASE_ADDR[];
extern uint8_t FW_LIMIT_ADDR[];

char *devID = "-----BEGIN PUBLIC KEY-----\n"
"MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEZegFpxcakc33TTv8EQFN32wPObeJCMa3yG2t5iWeyNJvIS1L+6gA+bOnUeliqt89pA/vqDiXzB8WQ495/aDVeA==\n"
"-----END PUBLIC KEY-----";

char *devsi = "-----BEGIN CERTIFICATE-----\n"
"MIIBbTCCARSgAwIBAgIFDg0MCwowCgYIKoZIzj0EAwIwNDESMBAGA1UEAwwJUklvVCBDb3JlMQswCQYDVQQGDAJVUzERMA8GA1UECgwITVNSX1RFU1QwHhcNMTcwMTAxMDAwMDAwWhcNMzcwMTAxMDAwMDAwWjA0MRIwEAYDVQQDDAlSSW9UIENvcmUxCzAJBgNVBAYMAlVTMREwDwYDVQQKDAhNU1JfVEVTVDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGXoBacXGpHN9007/BEBTd9sDzm3iQjGt8htreYlnsjSbyEtS/uoAPmzp1HpYqrfPaQP76g4l8wfFkOPef2g1XijEzARMA8GA1UdEwQIMAYBAf8CAQIwCgYIKoZIzj0EAwIDRwAwRAIgCQa7BBKKjpGvnyPgmzqH5gWQD9wwCOo6+sdKvNHk9IUCIB8Vxpg4ShdPWP/8gp6eU1Gba5W8ySMYbb2SbtNUmWWd\n"
"-----END CERTIFICATE-----";

char *aliascer = "-----BEGIN CERTIFICATE-----\n"
"MIICKTCCAdCgAwIBAgIFCgsMDQ4wCgYIKoZIzj0EAwIwNDESMBAGA1UEAwwJUklvVCBDb3JlMQswCQYDVQQGDAJVUzERMA8GA1UECgwITVNSX1RFU1QwHhcNMTcwMTAxMDAwMDAwWhcNMzcwMTAxMDAwMDAwWjA3MRUwEwYDVQQDDAxSSW9UX0Rldl90bXAxCzAJBgNVBAYMAlVTMREwDwYDVQQKDAhNU1JfVEVTVDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABNsHBaEi+XYmN4CitbE6bUGOB+Hi34xPv0f8NmrQvsOs3b2m9Of+jYNHgKBUjzT2Va6bgOa19W+N0ZMw5IitmRCjgcswgcgwFgYDVR0lAQH/BAwwCgYIKwYBBQUHAwIwga0GA1UdEQEB/wSBojCBn6CBnAYKKwYBBAGCN1kDATCBjQIBATBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGXoBacXGpHN9007/BEBTd9sDzm3iQjGt8htreYlnsjSbyEtS/uoAPmzp1HpYqrfPaQP76g4l8wfFkOPef2g1XgwLQYJYIZIAWUDBAIBBCDMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzDAKBggqhkjOPQQDAgNHADBEAiAJBrsEEoqOka+fI+CbOofmBZAP3DAI6jr6x0q80eT0hQIgTdKHdGOOFk/DUyLZrOnHHQD6AuaA1emhcVaFzS7y+hA=\n"
"-----END CERTIFICATE-----";

char *aliaspri = "-----BEGIN EC PRIVATE KEY-----\n"
"MHcCAQEEIHHaibHDezGS1JpKGtpMoBK5EG1FcpI3RbqxkEwGkGNboAoGCCqGSM49AwEHoUQDQgAE2wcFoSL5diY3gKK1sTptQY4H4eLfjE+/R/w2atC+w6zdvab05/6Ng0eAoFSPNPZVrpuA5rX1b43RkzDkiK2ZEA==\n"
"-----END EC PRIVATE KEY-----";

char *
RIoTGetDeviceID(
	unsigned int *len
)
{
	if (len)
		*len = g_DeviceIDPubLen;
	return g_DeviceIDPub;
}

char *
RIoTGetAliasKey(
	unsigned int *len
)
{ 
	if (len)
		*len = g_AliasKeyLen;
	return g_AliasKey;
}

char *
RIoTGetAliasCert(
	unsigned int *len
)
{
	if (len)
		*len = g_AKCertLen;
	return g_AKCert;
}

char *
RIoTGetDeviceCert(
	unsigned int *len
)
{
	if (len)
		*len = g_DICertLen;
	return g_DICert;
}
	

// The static data fields that make up the Alias Cert "to be signed" region
RIOT_X509_TBS_DATA x509AliasTBSData = { { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E },
                                       "STM32L4DICE", "MSR_TEST", "US",
                                       "170101000000Z", "370101000000Z",
                                       "STM32L4RIoTCore", "MSR_TEST", "US" };

// The static data fields that make up the DeviceID Cert "to be signed" region
RIOT_X509_TBS_DATA x509DeviceTBSData = { { 0x0E, 0x0D, 0x0C, 0x0B, 0x0A },
                                       "STM32L4DICE", "MSR_TEST", "US",
                                       "170101000000Z", "370101000000Z",
                                       "STM32L4DICE", "MSR_TEST", "US" };

extern int
AppStart(
void
);

static bool
RiotCore_Remediate(
    RIOT_STATUS status
)
// This is the remediation "handler" for RIoT Invariant Code.
{
    switch (status) {
        case RIOT_INVALID_STATE:
        case RIOT_INVALID_BOOT_MODE:
        case RIOT_INVALID_DEVICE_ID:
        case RIOT_INVALID_METADATA:
        case RIOT_LOAD_MODULE_FAILED:
            while (1) {
            }
        default:
            break;
    }

    // Indicate that this error is unhandled and we should reboot.
    return true;
}

void RiotStart(const uint8_t  *CDI, const uint16_t  CDILen)
// Entry point for RIoT Invariant Code
{

    RIOT_ECC_PUBLIC     deviceIDPub, aliasKeyPub;
    RIOT_ECC_PRIVATE    deviceIDPriv, aliasKeyPriv;
    RIOT_ECC_SIGNATURE  tbsSig;
    DERBuilderContext   derCtx;
    uint8_t            *base;
    uint32_t            length;
	  uint32_t						dcType;
  	
    // Validate Compound Device Identifier, pointer and length
    if (!(CDI) || (CDILen != RIOT_DIGEST_LENGTH)) {
        // Invalid state, remediate.
        Riot_Remediate(RiotCore, RIOT_INVALID_STATE);
    }

    // Don't use CDI directly
    RiotCrypt_Hash(cDigest, RIOT_DIGEST_LENGTH, CDI, CDILen);

    // Derive DeviceID key pair from CDI
    RiotCrypt_DeriveEccKey(&deviceIDPub,
                           &deviceIDPriv,
                           cDigest, RIOT_DIGEST_LENGTH,
                           (const uint8_t *)RIOT_LABEL_IDENTITY,
                           lblSize(RIOT_LABEL_IDENTITY));

    // Pickup start of FW image and calculate its length
    base = (uint8_t*)0x080F0000;
    length = (uint8_t*)0x080F000C - base;
		(void)printf("show info of FW image\r\n");
		(void)printf("base = %x, length = %d\r\n", base, length);

    // Measure FW, i.e., calculate FWID
    RiotCrypt_Hash(FWID, RIOT_DIGEST_LENGTH, base, length);

    // Combine CDI and FWID, result in cDigest
    RiotCrypt_Hash2(cDigest, RIOT_DIGEST_LENGTH,
                    cDigest, RIOT_DIGEST_LENGTH,
                    FWID, RIOT_DIGEST_LENGTH);

    // Derive Alias key pair from CDI and FWID
    RiotCrypt_DeriveEccKey(&aliasKeyPub,
                           &aliasKeyPriv,
                           cDigest, RIOT_DIGEST_LENGTH,
                           (const uint8_t *)RIOT_LABEL_ALIAS,
                           lblSize(RIOT_LABEL_ALIAS));
                                                     
    // Clean up potentially sensative data
    memset(cDigest, 0x00, RIOT_DIGEST_LENGTH);

    // Build the TBS (to be signed) region of Alias Key Certificate
    DERInitContext(&derCtx, derBuffer, DER_MAX_TBS);
    X509GetAliasCertTBS(&derCtx, &x509AliasTBSData,
                        &aliasKeyPub, &deviceIDPub,
                        FWID, RIOT_DIGEST_LENGTH);
                                                     
    // Sign the Alias Key Certificate's TBS region
    RiotCrypt_Sign(&tbsSig, derCtx.Buffer, derCtx.Position, &deviceIDPriv);

    // Generate Alias Key Certificate
    X509MakeAliasCert(&derCtx, &tbsSig);
                                                
    // Copy Alias Key Certificate
    length = sizeof(g_AKCert);
    DERtoPEM(&derCtx, CERT_TYPE, g_AKCert, &length);
		g_AKCertLen = length;

    // Clean up potentially sensative data
    memset(FWID, 0, sizeof(FWID));

    // Copy DeviceID Public
    DERInitContext(&derCtx, derBuffer, DER_MAX_TBS);
    X509GetDEREccPub(&derCtx, deviceIDPub);
		length = sizeof(g_DeviceIDPub);
    DERtoPEM(&derCtx, PUBLICKEY_TYPE, g_DeviceIDPub, &length);
		g_DeviceIDPubLen = length;

    // Copy Alias Key Pair
    DERInitContext(&derCtx, derBuffer, DER_MAX_TBS);
    X509GetDEREcc(&derCtx, aliasKeyPub, aliasKeyPriv);
		length = sizeof(g_AliasKey);
    DERtoPEM(&derCtx, ECC_PRIVATEKEY_TYPE, g_AliasKey, &length);
		g_AliasKeyLen = length;
		
    // We don't do CSRs on this device, this should be true.
    if(SelfSignedDeviceCert) {
			// Build the TBS (to be signed) region of DeviceID Certificate
        DERInitContext(&derCtx, derBuffer, DER_MAX_TBS);
        X509GetDeviceCertTBS(&derCtx, &x509DeviceTBSData, &deviceIDPub);

        // Sign the DeviceID Certificate's TBS region
        RiotCrypt_Sign(&tbsSig, derCtx.Buffer, derCtx.Position, &deviceIDPriv);

        // Generate DeviceID Certificate
        X509MakeDeviceCert(&derCtx, &tbsSig);
			
				// DeviceID Cert type
				dcType = CERT_TYPE;
		} else {            
        // Initialize, create CSR TBS region
        DERInitContext(&derCtx, derBuffer, DER_MAX_TBS);
        X509GetDERCsrTbs(&derCtx, &x509AliasTBSData, &deviceIDPub);

        // Sign the Alias Key Certificate's TBS region
        RiotCrypt_Sign(&tbsSig, derCtx.Buffer, derCtx.Position, &deviceIDPriv);

        // Create CSR for DeviceID
        X509GetDERCsr(&derCtx, &tbsSig);
			
			  // DeviceID CSR type
			  dcType = CERT_REQ_TYPE;
		}

		// Copy CSR/self-signed Cert
    length = sizeof(g_DICert);
    DERtoPEM(&derCtx, dcType, g_DICert, &length);
		g_DICertLen = length;
		
		// Clean up sensative data
		memset(&deviceIDPriv, 0, sizeof(RIOT_ECC_PRIVATE));
		
    // Transfer control to firmware
    //AppStart();
    
    // Must not return. If we do, DICE will trigger reset.
    return;
}
