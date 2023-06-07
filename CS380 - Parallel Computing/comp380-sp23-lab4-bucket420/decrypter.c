#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/des.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


/**
 * encrypt_des - encrypts a block of memory with the DES algorithm
 * @param key - character string containing encryption key
 * @param msg - array of bytes to encrypt
 * @param size - size of msg
 * @returns encrypted array of bytes
 */
char *encrypt_des(char *key, char *msg, int size) {
  static char*    res;
  int             n=0;
  DES_cblock      key2;
  DES_key_schedule schedule;

  res = (char *) malloc(size);

  /* Prepare the key for use with DES_cfb64_encrypt */
  memcpy(key2, key,8);
  DES_set_odd_parity(&key2);
  DES_set_key_checked(&key2, &schedule);

  /* Encryption occurs here */
  DES_cfb64_encrypt((unsigned char *) msg, (unsigned char *) res,
      size, &schedule, &key2, &n, DES_ENCRYPT);
  return (res);
}

/**
 * decrypt_des - decrypts a block of memory with the DES algorithm
 * @param key - character string containing decryption key
 * @param msg - array of bytes to decrypt
 * @param size - size of msg
 * @returns decrypted array of bytes
 */
char *decrypt_des(char *key, char *msg, int size) {
  static char*    res;
  int             n=0;

  DES_cblock      key2;
  DES_key_schedule schedule;

  res = (char *) malloc(size);

  /* Prepare the key for use with DES_cfb64_encrypt */
  memcpy(key2, key,8);
  DES_set_odd_parity(&key2);
  DES_set_key_checked(&key2, &schedule);

  /* Decryption occurs here */
  DES_cfb64_encrypt((unsigned char *) msg, (unsigned char *) res,
      size, &schedule, &key2, &n, DES_DECRYPT);
  return (res);
}


/*
 *  main function
 */
int main(int argc, char **argv) {
  char *buf = NULL, *cbuf = NULL;             // buffers for encrypt/decrypt
  char *cpass;                                // encrypted password
  char salt[3] = "AA";                        // crypt salt
  struct stat st;
  char *fileext = ".txt";                     // file extension for encrypted file
  int rfd, wfd;                               // file descriptors
  char *filename = NULL, *cfilename = NULL;   // plain and encrypted filenames
  int len, extlen;                            // buffer lengths


  /*
   * check arguments for sanity
   */
  if (argc != 4) {
    printf("\tusage: crypter <filename> <salt> <password>\n");
    exit(1);
  }

  /*
   * encrypt password string
   */
  if (strnlen(argv[2], sizeof(salt)) >= (sizeof(salt))) {
    printf("salt must be shorter than %lu characters\n", sizeof(salt));
    exit(1);
  }
  strncpy(salt, argv[2], sizeof(salt)-1);

  // get password from args
  char pass[strnlen(argv[3], 5) + 1];
  pass[sizeof(pass) - 1] = '\0';
  strncpy(pass, argv[3], sizeof(pass) - 1);

  // encrypt password
  cpass = DES_crypt(pass, salt);
  printf("crypted passwd is: \"%s\"\n", cpass);


  /*
   * read file, encrypt with password string and store in output file
   */
  cfilename = argv[1];
  len = strnlen(cfilename, 1024); // max length 1024
  extlen = strnlen(fileext, 1024);
  filename = malloc(len + 1);
  strncpy(filename, cfilename, len - extlen);
  strncat(filename, fileext, extlen); // append .txt to input file
  printf("source: %s decrypted: %s\n", cfilename, filename);

  // get input file size
  if (stat(cfilename, &st) != 0) {
    perror("stat");
    exit(1);
  }

  // allocate a buffer for the input file
  cbuf  = malloc(st.st_size+1);

  // open input file
  if ((rfd = open(cfilename, O_RDONLY)) == -1) {
    perror("open (reading)");
    exit(1);
  }

  // open output file
  if ((wfd = open(filename, O_CREAT|O_WRONLY|O_TRUNC, st.st_mode)) == -1) {
    perror("open (writing)");
    exit(1);
  }


  read(rfd, cbuf, st.st_size); // read entire file into buffer
  cbuf[st.st_size] = '\0';     // null terminate string

  // decrypted message
  buf = decrypt_des(pass, cbuf, st.st_size);

  // save it in output file
  write(wfd, buf, st.st_size);

  close(rfd);
  close(wfd);

  free(filename);
  free(buf);
  free(cbuf);
}
