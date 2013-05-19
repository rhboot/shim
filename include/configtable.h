/* definitions straight from TianoCore */

typedef UINT32 EFI_IMAGE_EXECUTION_ACTION;

#define EFI_IMAGE_EXECUTION_AUTHENTICATION      0x00000007 
#define EFI_IMAGE_EXECUTION_AUTH_UNTESTED       0x00000000
#define EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED     0x00000001
#define EFI_IMAGE_EXECUTION_AUTH_SIG_PASSED     0x00000002
#define EFI_IMAGE_EXECUTION_AUTH_SIG_NOT_FOUND  0x00000003
#define EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND      0x00000004
#define EFI_IMAGE_EXECUTION_POLICY_FAILED       0x00000005
#define EFI_IMAGE_EXECUTION_INITIALIZED         0x00000008

typedef struct {
  ///
  /// Describes the action taken by the firmware regarding this image.
  ///
  EFI_IMAGE_EXECUTION_ACTION    Action;
  ///
  /// Size of all of the entire structure.
  ///
  UINT32                        InfoSize;
  ///
  /// If this image was a UEFI device driver (for option ROM, for example) this is the 
  /// null-terminated, user-friendly name for the device. If the image was for an application, 
  /// then this is the name of the application. If this cannot be determined, then a simple 
  /// NULL character should be put in this position.
  /// CHAR16                    Name[];
  ///

  ///
  /// For device drivers, this is the device path of the device for which this device driver 
  /// was intended. In some cases, the driver itself may be stored as part of the system 
  /// firmware, but this field should record the device's path, not the firmware path. For 
  /// applications, this is the device path of the application. If this cannot be determined, 
  /// a simple end-of-path device node should be put in this position.
  /// EFI_DEVICE_PATH_PROTOCOL  DevicePath;
  ///

  ///
  /// Zero or more image signatures. If the image contained no signatures, 
  /// then this field is empty.
  ///
  ///EFI_SIGNATURE_LIST            Signature;
  UINT8 Data[];
} EFI_IMAGE_EXECUTION_INFO;

typedef struct {
  ///
  /// Number of EFI_IMAGE_EXECUTION_INFO structures.
  ///
  UINTN                     NumberOfImages; 
  ///
  /// Number of image instances of EFI_IMAGE_EXECUTION_INFO structures.
  ///
  EFI_IMAGE_EXECUTION_INFO  InformationInfo[];
} EFI_IMAGE_EXECUTION_INFO_TABLE;


void *
configtable_get_table(EFI_GUID *guid);
EFI_IMAGE_EXECUTION_INFO_TABLE *
configtable_get_image_table(void);
EFI_IMAGE_EXECUTION_INFO *
configtable_find_image(const EFI_DEVICE_PATH *DevicePath);
int
configtable_image_is_forbidden(const EFI_DEVICE_PATH *DevicePath);

