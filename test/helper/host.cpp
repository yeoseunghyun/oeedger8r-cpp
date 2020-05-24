// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/edger8r/host.h>

#include "enclave_impl.h"

thread_local oe_enclave_t* _enclave;

typedef void (*oe_ecall_func_t)(
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written);

extern "C" oe_ecall_func_t __oe_ecalls_table[];
extern "C" size_t __oe_ecalls_table_size;

extern "C" oe_result_t oe_call_enclave_function(
    oe_enclave_t* enclave,
    uint32_t function_id,
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written)
{
    oe_result_t result = OE_FAILURE;
    oe_enclave_t* previous_enclave = _enclave;
    _enclave = enclave;

    if (!_enclave->is_outside_enclave(input_buffer, input_buffer_size) ||
        !_enclave->is_outside_enclave(output_buffer, output_buffer_size))
    {
        result = OE_INVALID_PARAMETER;
        goto done;
    }

    {
        void* enc_input_buffer = enclave->malloc(input_buffer_size);
        memcpy(enc_input_buffer, input_buffer, input_buffer_size);
        void* enc_output_buffer = enclave->malloc(output_buffer_size);
        memset(enc_output_buffer, 0, output_buffer_size);
        size_t* enc_output_bytes_written =
            (size_t*)enclave->malloc(sizeof(size_t));

        __oe_ecalls_table[function_id](
            enc_input_buffer,
            input_buffer_size,
            enc_output_buffer,
            output_buffer_size,
            enc_output_bytes_written);

        *output_bytes_written = *enc_output_bytes_written;
        memcpy(output_buffer, enc_output_buffer, output_buffer_size);

        enclave->free(enc_output_bytes_written);
        enclave->free(enc_output_buffer);
        enclave->free(enc_input_buffer);
        result = *(oe_result_t*)output_buffer;
    }

done:
    _enclave = previous_enclave;

    if (result == OE_INVALID_PARAMETER)
        printf("ecall returned OE_INVALID_PARAMETER\n");

    return result;
}

extern "C" oe_result_t oe_create_enclave(
    const char* path,
    oe_enclave_type_t type,
    uint32_t flags,
    const oe_enclave_setting_t* settings,
    uint32_t setting_count,
    const oe_ocall_func_t* ocall_table,
    uint32_t num_ocalls,
    oe_enclave_t** enclave)
{
    OE_UNUSED(path);
    OE_UNUSED(type);
    OE_UNUSED(flags);
    OE_UNUSED(settings);
    OE_UNUSED(setting_count);

    *enclave = new _oe_enclave(ocall_table, num_ocalls);
    return OE_OK;
}

extern "C" oe_result_t oe_terminate_enclave(oe_enclave_t* enclave)
{
    delete enclave;
    return OE_OK;
}

extern "C" uint32_t oe_get_create_flags(void)
{
    return 0;
}
