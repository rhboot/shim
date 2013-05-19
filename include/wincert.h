#ifndef _INC_WINCERT_H
#define _INC_WINCERT_H

///
/// The WIN_CERTIFICATE structure is part of the PE/COFF specification.
///
typedef struct {
  ///
  /// The length of the entire certificate,  
  /// including the length of the header, in bytes.                                
  ///
  UINT32  dwLength;
  ///
  /// The revision level of the WIN_CERTIFICATE 
  /// structure. The current revision level is 0x0200.                                   
  ///
  UINT16  wRevision;
  ///
  /// The certificate type. See WIN_CERT_TYPE_xxx for the UEFI      
  /// certificate types. The UEFI specification reserves the range of 
  /// certificate type values from 0x0EF0 to 0x0EFF.                          
  ///
  UINT16  wCertificateType;
  ///
  /// The following is the actual certificate. The format of   
  /// the certificate depends on wCertificateType.
  ///
  /// UINT8 bCertificate[ANYSIZE_ARRAY];
  ///
} WIN_CERTIFICATE;


#endif
