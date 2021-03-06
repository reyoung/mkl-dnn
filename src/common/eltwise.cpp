/*******************************************************************************
* Copyright 2016-2017 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <assert.h>
#include "mkldnn.h"

#include "c_types_map.hpp"
#include "type_helpers.hpp"
#include "utils.hpp"

using namespace mkldnn::impl;
using namespace mkldnn::impl::utils;
using namespace mkldnn::impl::status;
using namespace mkldnn::impl::prop_kind;
using namespace mkldnn::impl::alg_kind;
using namespace mkldnn::impl::types;

namespace {
status_t eltwise_desc_init(eltwise_desc_t *eltwise_desc, prop_kind_t prop_kind,
        alg_kind_t alg_kind, const memory_desc_t *data_desc,
        const memory_desc_t *diff_data_desc, double alpha, double beta) {
    bool args_ok = true
        && !any_null(eltwise_desc, data_desc)
        && one_of(prop_kind, forward_training, forward_inference,
                backward_data)
        && one_of(alg_kind, eltwise_relu, eltwise_tanh, eltwise_elu)
        && implication(prop_kind == backward_data, diff_data_desc != nullptr);
    if (!args_ok) return invalid_arguments;

    eltwise_desc_t ed = {};
    ed.primitive_kind = primitive_kind::eltwise;
    ed.prop_kind = prop_kind;
    ed.alg_kind = alg_kind;

    ed.data_desc = *data_desc;
    ed.diff_data_desc =
        (ed.prop_kind == backward_data) ? *diff_data_desc : zero_md();

    ed.alpha = alpha;
    ed.beta = beta;
    ed.negative_slope = ed.alpha;

    bool consistency = true
        && implication(ed.prop_kind == backward_data,
                array_cmp(ed.diff_data_desc.dims, ed.data_desc.dims,
                    ed.diff_data_desc.ndims));
    if (!consistency) return invalid_arguments;

    *eltwise_desc = ed;
    return success;
}
}

status_t mkldnn_eltwise_forward_desc_init(eltwise_desc_t *eltwise_desc,
        prop_kind_t prop_kind, alg_kind_t alg_kind,
        const memory_desc_t *data_desc, double alpha, double beta) {
    if (!one_of(prop_kind, forward_training, forward_inference))
        return invalid_arguments;
    return eltwise_desc_init(eltwise_desc, prop_kind, alg_kind, data_desc,
            nullptr, alpha, beta);
}

status_t mkldnn_eltwise_backward_desc_init(eltwise_desc_t *eltwise_desc,
        alg_kind_t alg_kind, const memory_desc_t *diff_data_desc,
        const memory_desc_t *data_desc, double alpha, double beta) {
    return eltwise_desc_init(eltwise_desc, backward_data, alg_kind, data_desc,
            diff_data_desc, alpha, beta);
}

status_t mkldnn_relu_forward_desc_init(eltwise_desc_t *relu_desc,
        prop_kind_t prop_kind, const memory_desc_t *data_desc,
        double negative_slope) {
    return mkldnn_eltwise_forward_desc_init(relu_desc, prop_kind, eltwise_relu,
            data_desc, negative_slope, 0.);
}

status_t mkldnn_relu_backward_desc_init(eltwise_desc_t *relu_desc,
        const memory_desc_t *diff_data_desc, const memory_desc_t *data_desc,
        double negative_slope) {
    return mkldnn_eltwise_backward_desc_init(relu_desc, eltwise_relu,
            diff_data_desc, data_desc, negative_slope, 0.);
}


// vim: et ts=4 sw=4 cindent cino^=l0,\:0,N-s
