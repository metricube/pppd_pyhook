/*******************************************************************
 * main.c
 *
 * A plugin for pppd that allows for pppd hooks to be implemented in python
 *
 ********************************************************************/

#include <Python.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pppd/pppd.h>
#include <pppd/pathnames.h>
#include <pppd/fsm.h>
#include <pppd/ipcp.h>

#include "main.h"

#define _PATH_PYHOOKS	"hooks.py"
#define _PY_MODULE	"hooks"

char pppd_version[] = VERSION;
PyObject *pModule = NULL;

// Buffers so our logs are readable
char stderr_buf[256] = {0};
char stdout_buf[256] = {0};

/*
 * Initialize the PPPD plugin
 */
int plugin_init() {
	pyinit();

	/* Hooks */
	if (has_PyFunc("get_secret_for_user")) {
		chap_verify_hook = chap_verify_wrapper;
		info("pyhook: chap_check_hook => get_secret_for_user");
	}

	if (has_PyFunc("chap_check_hook")) {
		chap_check_hook = chap_check_wrapper;
		info("pyhook: added hook chap_check_hook");
	}

	if (has_PyFunc("allowed_address_hook")) {
		allowed_address_hook = allowed_address_wrapper;
		info("pyhook: added hook allowed_address_hook");
	}

	/* Notifications */
	if (has_PyFunc("ip_up_notifier")) {
		add_notifier(&ip_up_notifier, py_ip_up_notifier, "ip_up_notifier");
		info("pyhook: added notifier ip_up_notifier");
	}

	if (has_PyFunc("ip_down_notifier")) {
		add_notifier(&ip_down_notifier, generic_notifier_wrapper, "ip_down_notifier");
		info("pyhook: added notifier ip_down_notifier");
	}

	if (has_PyFunc("auth_up_notifier")) {
		add_notifier(&auth_up_notifier, generic_notifier_wrapper, "auth_up_notifier");
		info("pyhook: added notifier auth_up_notifier");
	}

	if (has_PyFunc("link_down_notifier")) {
		add_notifier(&link_down_notifier, generic_notifier_wrapper, "link_down_notifier");
		info("pyhook: added notifier link_down_notifier");
	}

	info("pyhook: plugin initialized.");
}


/*
 * Method to redirect python stderr / stdout to pppd logging.  This allows
 * for standard Python error reporting output.
 */
static PyObject *
pppd_stderr_write(PyObject *self, PyObject *args) {
	char *what;
	int len, buf_len;
	int newline = 0;

	if (!PyArg_ParseTuple(args, "s", &what))
		return NULL;

	len = strlen(what);
	if(len > 0 && what[len-1] == '\n') {
		newline = 1;
	}

	if(newline || (strlen(stderr_buf) + len) > sizeof stderr_buf) {
		if(strlen(stderr_buf) > 0) {
			error("%s%s", stderr_buf, what);
		} else {
			error("%s", what);
		}
		stderr_buf[0] = '\0';
	} else {
		buf_len = strlen(stderr_buf);
		strncat(stderr_buf + buf_len, what, len);
	}

	return Py_BuildValue("");
}

static PyMethodDef pppd_stderr_methods[] = {
	{"write", pppd_stderr_write, METH_VARARGS, "Write STDERR"},
	{NULL, NULL, 0, NULL}
};

static PyObject *
pppd_stdout_write(PyObject *self, PyObject *args) {
	char *what;
	int len, buf_len;
	int newline = 0;

	if (!PyArg_ParseTuple(args, "s", &what))
		return NULL;

	len = strlen(what);
	if(len > 0 && what[len-1] == '\n') {
		newline = 1;
	}

	if(newline || (strlen(stdout_buf) + len) > sizeof stdout_buf) {
		if(strlen(stdout_buf) > 0) {
			error("%s%s", stdout_buf, what);
		} else {
			error("%s", what);
		}
		stdout_buf[0] = '\0';
	} else {
		buf_len = strlen(stdout_buf);
		strncat(stdout_buf + buf_len, what, len);
	}

	return Py_BuildValue("");
}

static PyMethodDef pppd_stdout_methods[] = {
	{"write", pppd_stdout_write, METH_VARARGS, "Write STDOUT"},
	{NULL, NULL, 0, NULL}
};



/*
 * Load our python module
 */
int pyinit() {
	char *err;
	char *argv[] = { "pppd_pyhook", NULL };
	Py_Initialize();
	PySys_SetArgvEx(0, argv, 0);

	// Redirect stdout/stderr - this will also give us our
	// python error debugging
	PyObject *m_stdout = Py_InitModule("pppd_stdout", pppd_stdout_methods);
	if (m_stdout == NULL) {
		error("Unable to init pppd_stdout");
	} else {
#ifdef DEBUG
		info("Set stdout to m_stdout");
#endif

		PySys_SetObject("stdout", m_stdout);
	}

	PyObject *m_stderr = Py_InitModule("pppd_stderr", pppd_stderr_methods);
	if (m_stderr == NULL) {
		error("Unable to init pppd_stderr");
	} else {
#ifdef DEBUG
		info("Set stderr to m_stderr");
#endif

		PySys_SetObject("stderr", m_stderr);
	}

	// Load the /etc/ppp/hooks.py file
	PyObject *sysPath = PySys_GetObject("path");
	PyObject *path = PyString_FromString(_PATH_VARRUN);
	int result = PyList_Insert(sysPath, 0, path);
	pModule = PyImport_ImportModule(_PY_MODULE);

	if (pModule == NULL) {
		PyErr_Print();
		Py_Finalize();
		exit(1);
	}
}

/*
 * Get a python function if it exists
 */
PyObject* get_PyFunc(char *name) {
#ifdef DEBUG
	info("get_PyFunc(%s)", name);
#endif

	PyObject *pFunc;
	char *err;
	pFunc = PyObject_GetAttrString(pModule, name);

	if (pFunc && PyCallable_Check(pFunc)) {
		return pFunc;
	}

	if (PyErr_Occurred()) {
		PyErr_Print();
	} else {
		error("Cannot find python function \"%s\"", name);
	}
	exit(1);
}

/*
 * Check if python function exists
 */
int has_PyFunc(char *name) {
#ifdef DEBUG
	info("has_PyFunc(%s)", name);
#endif

	PyObject *pFunc;
	pFunc = PyObject_GetAttrString(pModule, name);

	if (pFunc && PyCallable_Check(pFunc)) {
		Py_XDECREF(pFunc);
		return 1;
	}
	return 0;
}

/*
 *  Calls an external python function "get_secret_for_user"
 *  to get the expected secret.  If the secret matches what the
 *  user has provided - auth will proceed.  If not auth will
 *  fail.
 *
 *  The chap_verify_wrapper supplies an implementation for the
 *  pppd hook chap_verify_hook to implement this functionality.
 *
 */
static int chap_verify_wrapper(char *name, char *ourname, int id,
		struct chap_digest_type *digest,
		unsigned char *challenge, unsigned char *response,
		char *message, int message_space)
{

	int ok = 0;
	char* secret;
	PyObject *pFunc, *pValue, *pArgs;

	pFunc = get_PyFunc("get_secret_for_user");
	pArgs = PyTuple_New(2);
	pValue = PyString_FromString(name);
	PyTuple_SetItem(pArgs, 0, pValue);
	pValue = PyString_FromString(ipparam);
	PyTuple_SetItem(pArgs, 1, pValue);

	// call the function
	pValue = PyObject_CallObject(pFunc, pArgs);
	Py_DECREF(pArgs);
	if (pValue == NULL) {
		PyErr_Print();
	} else {
		secret = PyString_AsString(pValue);
		int secret_len = strlen(secret);

		// reuse the digest to calculate a peer response
		ok = digest->verify_response(id, name, secret, secret_len, challenge,
				response, message, message_space);

		Py_DECREF(pValue);
	}

#ifdef DEBUG
	info("Checking CHAP with name:%s ourname:%s id:%d digest_code:%x message:%s",
			name, ourname, id, digest->code, challenge, response, message);
#endif

	Py_XDECREF(pFunc);
	return ok;
}

/* Tells pppd that we will try to authenticate the peer, and not to
 * worry about looking in /etc/ppp/*-secrets
 */
static int chap_check_wrapper(void) {
	int result = 0;
	PyObject *pFunc, *pValue, *pArgs;

	pFunc = get_PyFunc("chap_check_hook");
	pArgs = PyTuple_New(0);
	pValue = PyObject_CallObject(pFunc, pArgs);
	Py_DECREF(pArgs);
	if (pValue == NULL) {
		PyErr_Print();
	} else {
		result = (pValue == Py_True);
		Py_DECREF(pValue);
	}

	Py_XDECREF(pFunc);
	return result;
}

/*
 * Allow any address from available pool
 */
static int allowed_address_wrapper(u_int32_t addr) {
	int result = 0;
	PyObject *pFunc, *pValue, *pArgs;

	pFunc = get_PyFunc("allowed_address_hook");
	pArgs = PyTuple_New(1);
	pValue = PyInt_FromSize_t(addr);
	PyTuple_SetItem(pArgs, 0, pValue);

	pValue = PyObject_CallObject(pFunc, pArgs);
	Py_DECREF(pArgs);
	if (pValue == NULL) {
		PyErr_Print();
	} else {
		result = (pValue == Py_True);
		Py_DECREF(pValue);
	}

	Py_XDECREF(pFunc);
	return result;
}


static void generic_notifier_wrapper(void *opaque, int arg) {
	PyObject *pFunc, *pValue, *pArgs;
	pFunc = get_PyFunc((char*)opaque);
	pArgs = PyTuple_New(1);
	pValue = PyInt_FromSize_t(arg);
	PyTuple_SetItem(pArgs, 0, pValue);

	pValue = PyObject_CallObject(pFunc, pArgs);
	Py_DECREF(pArgs);
	if (pValue == NULL) {
		PyErr_Print();
	} else {
		Py_DECREF(pValue);
	}

	Py_XDECREF(pFunc);
}

static void py_ip_up_notifier(void *pyfunc, int arg) {
	ipcp_options opts = ipcp_gotoptions[0];
	ipcp_options peer = ipcp_hisoptions[0];

	PyObject *pArgs = PyTuple_New(3);

	PyObject *pIFName = PyString_FromString(ifname);
	PyTuple_SetItem(pArgs, 0, pIFName);

	char *ouraddr = inet_ntoa((*(struct in_addr *) &opts.ouraddr));
	PyObject *pOurAddr = PyString_FromString(ouraddr);
	PyTuple_SetItem(pArgs, 1, pOurAddr);

	char *hisaddr = inet_ntoa((*(struct in_addr *) &opts.hisaddr));
	PyObject *pHisAddr = PyString_FromString(hisaddr);
	PyTuple_SetItem(pArgs, 2, pHisAddr);

	PyObject *pFunc = get_PyFunc((char*)pyfunc);
	PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
	Py_DECREF(pArgs);
	if (pValue == NULL) {
		PyErr_Print();
	} else {
		Py_DECREF(pValue);
	}
	Py_XDECREF(pFunc);
}
