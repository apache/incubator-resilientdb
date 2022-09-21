#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "crypto/signature_verifier.h"

using namespace resdb;

CertificateInfo ReadData(const std::string& file_name) {
  int fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd < 0) {
    printf("open file %s fail %s\n", file_name.c_str(), strerror(errno));
  }
  assert(fd >= 0);

  std::string res;
  int read_len = 0;
  char tmp[1024];
  while (true) {
    read_len = read(fd, tmp, sizeof(tmp));
    if (read_len <= 0) {
      break;
    }
    res.append(tmp, read_len);
  }
  close(fd);
  CertificateInfo info;
  assert(info.ParseFromString(res));
  return info;
}

KeyInfo ReadKey(const std::string& file_name) {
  int fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd < 0) {
    printf("open file %s fail %s\n", file_name.c_str(), strerror(errno));
  }
  assert(fd >= 0);

  std::string res;
  int read_len = 0;
  char tmp[1024];
  while (true) {
    read_len = read(fd, tmp, sizeof(tmp));
    if (read_len <= 0) {
      break;
    }
    res.append(tmp, read_len);
  }
  close(fd);
  KeyInfo key;
  assert(key.ParseFromString(res));
  return key;
}

int main(int argc, char** argv) {
  std::string cert_path, private_key_path;
  if (argc < 2) {
    printf("<private key path> <cert path>\n");
    exit(0);
  }
  private_key_path = argv[1];
  cert_path = argv[2];

  // Read the private key.
  KeyInfo private_key = ReadKey(private_key_path);

  // Read the certificate.
  CertificateInfo info = ReadData(cert_path);

  // Get certificate using the key from administor.
  SignatureVerifier verifier(private_key, info);
  auto resp_info = verifier.VerifyKey(info.public_key().public_key_info(),
                                      info.public_key().certificate());
  assert(resp_info);
}
