#include <pppd/chap-new.h>
#include <sys/types.h>

static int chap_verify_wrapper(char *name, char *ourname, int id,
			struct chap_digest_type *digest,
			unsigned char *challenge, unsigned char *response,
			char *message, int message_space);

static int chap_check_wrapper(void);
static int allowed_address_wrapper(u_int32_t addr);
static void generic_notifier_wrapper(void *opaque, int arg);
static void py_ip_up_notifier(void *pyfunc, int arg);
PyObject* get_PyFunc(char *name);
