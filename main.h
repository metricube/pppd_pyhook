
#include "chap-new.h"
#include <sys/types.h>

static int chap_verify_wrapper(char *name, char *ourname, int id,
			struct chap_digest_type *digest,
			unsigned char *challenge, unsigned char *response,
			char *message, int message_space);

static int chap_check_wrapper(void);
static int allowed_address_wrapper(u_int32_t addr);

char* get_PyError();
PyObject* get_PyFunc(char *name);
