#include "socket_client.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static void* SocketClient_worker(void* ctx) {
    SocketClient* self = (SocketClient*) ctx;

    int read_bytes;
    int available;
    int offset = 0;
    while (self->run) {
        available = Buffer_getWriteable(self->outputBuffer);
        if (available > 1024) available = 1024;
        read_bytes = recv(self->socket, Buffer_getWritePointer(self->outputBuffer) + offset, available * SOCKET_ITEM_SIZE - offset, 0);
        if (read_bytes <= 0) {
            self->run = false;
        } else {
            offset = (offset + read_bytes) % SOCKET_ITEM_SIZE;
            Buffer_advance(self->outputBuffer, (offset + read_bytes) / SOCKET_ITEM_SIZE);
        }
    }

    return NULL;
}

static PyObject* SocketClient_start(SocketClient* self) {
    if (self->outputBuffer == NULL) {
        Py_RETURN_NONE;
    } else {
        self->run = true;

        if (pthread_create(&self->worker, NULL, SocketClient_worker, self) != 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }

        pthread_setname_np(self->worker, "pycsdr SocketCl");

        Py_RETURN_NONE;
    }
}

static PyObject* SocketClient_stop(SocketClient* self, PyObject* Py_UNUSED(ignored)) {
    self->run = false;
    if (self->worker != 0) {
        void* retval = NULL;
        pthread_join(self->worker, retval);
    }
    self->worker = 0;
    Py_RETURN_NONE;
}

static int SocketClient_clear(SocketClient* self) {
    SocketClient_stop(self, Py_None);
    close(self->socket);
    if (self->outputBuffer != NULL) Py_DECREF(self->outputBuffer);
    self->outputBuffer = NULL;
    return 0;
}

static int SocketClient_init(SocketClient* self, PyObject* args, PyObject* kwds) {
    static char* kwlist[] = {"port", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &self->port)) {
        return -1;
    }

    struct sockaddr_in remote;

    if ((self->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(self->port);
    remote.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(self->socket, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    return 0;
}

static PyObject* SocketClient_setOutput(SocketClient* self, PyObject* args, PyObject* kwds) {
    if (SocketClient_stop(self, Py_None) == NULL) {
        return NULL;
    }

    if (self->outputBuffer != NULL) Py_DECREF(self->outputBuffer);

    static char* kwlist[] = {"buffer", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist, getApiType("Buffer"), &self->outputBuffer))
        return NULL;

    Py_INCREF(self->outputBuffer);

    Buffer_setItemSize(self->outputBuffer, sizeof(complexf));

    return SocketClient_start(self);
}

static PyMethodDef SocketClient_methods[] = {
    {"setOutput", (PyCFunction) SocketClient_setOutput, METH_VARARGS | METH_KEYWORDS,
     "set the output buffer"
    },
    {"stop", (PyCFunction) SocketClient_stop, METH_NOARGS,
     "stop processing"
    },
    {NULL}  /* Sentinel */
};

static PyType_Slot SocketClientSlots[] = {
    {Py_tp_init, SocketClient_init},
    {Py_tp_clear, SocketClient_clear},
    {Py_tp_methods, SocketClient_methods},
    {0, 0}
};

PyType_Spec SocketClientSpec = {
    "pycsdr.modules.SocketClient",
    sizeof(SocketClient),
    0,
    Py_TPFLAGS_DEFAULT,
    SocketClientSlots
};
