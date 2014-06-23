/*******************************************************************
* main.c
*
* CHAP via Shell plugin for pppd. Validates the supplied username and 
* password via a shell script.
*
********************************************************************/

#include <Python.h>
#include "pppd.h"
#include "pathnames.h"
#include "main.h"
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>

#define _PATH_PYHOOKS		"hooks.py"
#define _PY_MODULE		"hooks"
#define NO_ERROR		"No error message available"

char pppd_version[] = VERSION;
PyObject *pModule = NULL;

/*
 * Initialize the PPPD plugin
 */
int plugin_init() {
    pyinit();

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
    info("pyhook: plugin initialized.");
}

/*
 * Load our python module 
 */
int pyinit() {
    char *err;
    Py_Initialize();

    PyObject *sysPath = PySys_GetObject("path");
    PyObject *path = PyString_FromString(_PATH_VARRUN);
    int result = PyList_Insert(sysPath, 0, path);
    pModule = PyImport_ImportModule(_PY_MODULE);

    if (pModule == NULL) {
	err = get_PyError();	
	error("Failed to load Python module %s%s (%s)", _PATH_VARRUN, _PATH_PYHOOKS, err);
	Py_Finalize();
	exit(1);
    }	
}


/*
 * Get the Python error message
 * Return char* err message.  Never NULL
 */
char* get_PyError() {
    char* err;
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    if(ptraceback != NULL) {
	err = PyString_AsString(pvalue);
	if(err != NULL) {
	    return err;
	}
    }
    return NO_ERROR;
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
	err = get_PyError();
        error("Error loading python function \"%s\" (%s)", name, err);
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
    char* err;
    PyObject *pFunc, *pValue, *pArgs;

    pFunc = get_PyFunc("get_secret_for_user");
    pArgs = PyTuple_New(1);
    pValue = PyString_FromString(name);
    PyTuple_SetItem(pArgs, 0, pValue);

    // call the function
    pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
    if (pValue == NULL) {
	err = get_PyError();
	error("Call to \"get_secret_for_user\" failed (%s)", err);
    } else {
        secret = PyString_AsString(pValue);
        info("Result of call: %s\n", secret);

	// We got a result - calculate the response as necessary
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
    char* err;
    PyObject *pFunc, *pValue, *pArgs;

    pFunc = get_PyFunc("chap_check_hook");
    pArgs = PyTuple_New(0);
    pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
    if (pValue == NULL) {
        err = get_PyError();
        error("Call to \"chap_check_hook\" failed (%s)", err);
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
    char* err;
    PyObject *pFunc, *pValue, *pArgs;

    pFunc = get_PyFunc("allowed_address_hook");
    pArgs = PyTuple_New(1);
    pValue = PyInt_FromSize_t(addr);
    PyTuple_SetItem(pArgs, 0, pValue);
    
    pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
    if (pValue == NULL) {
        err = get_PyError();
        error("Call to \"allowed_address_hook\" failed (%s)", err);
    } else {
	result = (pValue == Py_True);
	Py_DECREF(pValue);
    }

    Py_XDECREF(pFunc);
    return result;
}


