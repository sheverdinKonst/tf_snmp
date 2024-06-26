
# To compile on SunOS: add "-lsocket -lnsl" to LDFLAGS
# To compile on MinGW: add "-lws2_32" to LDFLAGS
# To compile with PKCS11: add "-lpkcs11-helper" to LDFLAGS

CFLAGS	+= -I../include -D_FILE_OFFSET_BITS=64 -Wall -W -Wdeclaration-after-statement \
			-Wno-unused-function -Wno-unused-value

OFLAGS	= -O
LDFLAGS	+= -L../library -lpolarssl $(SYS_LDFLAGS)

APPS =	test_suite_aes			test_suite_arc4			\
		test_suite_base64		test_suite_camellia		\
		test_suite_cipher.aes	test_suite_cipher.camellia	\
		test_suite_cipher.des	test_suite_debug		\
		test_suite_des			test_suite_dhm			\
		test_suite_error		test_suite_hmac_shax	\
		test_suite_md			test_suite_mdx			\
		test_suite_mpi			test_suite_pkcs1_v21	\
		test_suite_rsa			test_suite_shax			\
		test_suite_x509parse	test_suite_xtea			\
		test_suite_version

.SILENT:

all: $(APPS)

test_suite_cipher.aes.c : suites/test_suite_cipher.function suites/test_suite_cipher.aes.data scripts/generate_code.pl suites/helpers.function
	echo   "  Generate	$@"
	scripts/generate_code.pl suites test_suite_cipher test_suite_cipher.aes

test_suite_cipher.camellia.c : suites/test_suite_cipher.function suites/test_suite_cipher.camellia.data scripts/generate_code.pl suites/helpers.function
	echo   "  Generate	$@"
	scripts/generate_code.pl suites test_suite_cipher test_suite_cipher.camellia

test_suite_cipher.des.c : suites/test_suite_cipher.function suites/test_suite_cipher.des.data scripts/generate_code.pl suites/helpers.function
	echo   "  Generate	$@"
	scripts/generate_code.pl suites test_suite_cipher test_suite_cipher.des

%.c : suites/%.function suites/%.data scripts/generate_code.pl suites/helpers.function
	echo   "  Generate	$@"
	scripts/generate_code.pl suites $* $*

test_suite_aes: test_suite_aes.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_arc4: test_suite_arc4.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_base64: test_suite_base64.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_camellia: test_suite_camellia.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_cipher.aes: test_suite_cipher.aes.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_cipher.camellia: test_suite_cipher.camellia.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_cipher.des: test_suite_cipher.des.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_des: test_suite_des.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_dhm: test_suite_dhm.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_error: test_suite_error.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_hmac_shax: test_suite_hmac_shax.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_md: test_suite_md.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_mdx: test_suite_mdx.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_mpi: test_suite_mpi.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_pkcs1_v21: test_suite_pkcs1_v21.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_rsa: test_suite_rsa.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_shax: test_suite_shax.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_x509parse: test_suite_x509parse.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_xtea: test_suite_xtea.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_debug: test_suite_debug.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

test_suite_version: test_suite_version.c ../library/libpolarssl.a
	echo   "  CC    	$@.c"
	$(CC) $(CFLAGS) $(OFLAGS) $@.c	$(LDFLAGS) -o $@

clean:
	rm -f $(APPS) *.c

check: $(APPS)
	echo "Running checks (Success if all tests PASSED)"
	for i in $(APPS);														\
	do																		\
		echo " - $${i}";													\
		./$${i} | grep -v 'PASS$$' | grep -v -- '-----' | grep -v '^$$';	\
		echo "";															\
	done
