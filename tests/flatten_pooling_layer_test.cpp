//   OpenNN: Open Neural Networks Library
//   www.opennn.net
//
//   F L A T T E N   P O O L I N G   L A Y E R   T E S T   C L A S S
//
//   Artificial Intelligence Techniques SL
//   artelnics@artelnics.com

#include "flatten_pooling_layer_test.h"

static constexpr type NUMERIC_LIMITS_MIN_4DIGIT = static_cast<type>(1e-4);

template<size_t DIM>
static bool is_equal(const Tensor<type, DIM>& expected_output, const Tensor<type, DIM>& output)
{
    Eigen::array<Index, DIM> dims;
    std::iota(begin(dims), end(dims), 0U);
    Tensor<type, DIM> abs_diff = (output - expected_output).abs();
    Tensor<bool, DIM> cmp_res = abs_diff < NUMERIC_LIMITS_MIN_4DIGIT;
    Tensor<bool, 0> ouput_equals_expected_output = cmp_res.reduce(dims, Eigen::internal::AndReducer());
    return ouput_equals_expected_output();
}

template<size_t DIM>
static constexpr Eigen::array<Index, DIM> t1d2array(const Tensor<Index, 1>& oned_tensor)
{
    return [&]<typename T, T...ints>(std::integer_sequence<T, ints...> int_seq)
    {
        Eigen::array<Index, DIM> ret({oned_tensor(ints)...});
        return ret;
    }(std::make_index_sequence<DIM>());
}

FlattenPoolingLayerTest::FlattenPoolingLayerTest() 
{
}


FlattenPoolingLayerTest::~FlattenPoolingLayerTest()
{
}

void FlattenPoolingLayerTest::test_pooling_flatten_forward_propagate()
{
    cout << "test_pooling_flatten_forward_propagate\n";

    bool switch_train = true;

    const Index numb_of_input_rows = 4;
    const Index numb_of_input_cols = 4;
    const Index numb_of_input_chs = 2;
    const Index numb_of_input_images = 2;

    Tensor<Index, 1> pooling_input_dimension(4);
    pooling_input_dimension[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    pooling_input_dimension[Convolutional4dDimensions::channel_index] = numb_of_input_chs;
    pooling_input_dimension[Convolutional4dDimensions::row_index] = numb_of_input_rows;
    pooling_input_dimension[Convolutional4dDimensions::column_index] = numb_of_input_cols;

    const Index pool_row_size = 2;
    const Index pool_col_size = 2;
    Tensor<Index, 1> pool_dimension(2);
    pool_dimension.setValues({
        pool_row_size,
        pool_col_size
    });

    const Index stride = 2;

    PoolingLayer pooling_layer(pooling_input_dimension, pool_dimension);
    pooling_layer.set_row_stride(stride);
    pooling_layer.set_column_stride(stride);
    pooling_layer.set_pooling_method(PoolingLayer::PoolingMethod::AveragePooling);

    Tensor<Index, 1> flatten_layer_input_dimensions = pooling_layer.get_outputs_dimensions();
    
    FlattenLayer flatten_layer(flatten_layer_input_dimensions);

    Tensor<type, 4> input(t1d2array<4>(pooling_input_dimension));
    //image 1
    input(0, 0, 0, 0) = type(1);
    input(0, 1, 0, 0) = type(2);
    input(0, 0, 1, 0) = type(1);
    input(0, 1, 1, 0) = type(2);
    input(0, 0, 2, 0) = type(3);
    input(0, 1, 2, 0) = type(4);
    input(0, 0, 3, 0) = type(3);
    input(0, 1, 3, 0) = type(4);
    input.chip(0, Convolutional4dDimensions::sample_index).
        chip(1, Convolutional4dDimensions::row_index - 1) = 
        input.chip(0, Convolutional4dDimensions::sample_index).chip(0, Convolutional4dDimensions::row_index - 1);
    input.chip(0, Convolutional4dDimensions::sample_index).chip(2, Convolutional4dDimensions::row_index - 1) = 
        input.chip(0, Convolutional4dDimensions::sample_index).chip(1, Convolutional4dDimensions::row_index - 1) + type(4);
    input.chip(0, Convolutional4dDimensions::sample_index).
        chip(3, Convolutional4dDimensions::row_index - 1) = 
        input.chip(0, Convolutional4dDimensions::sample_index).chip(2, Convolutional4dDimensions::row_index - 1);
    //image 2
    input.chip(1, Convolutional4dDimensions::sample_index) = input.chip(0, Convolutional4dDimensions::sample_index) + type(8);

    FlattenLayerForwardPropagation flatten_layer_forward_propagation(
        numb_of_input_images, &flatten_layer);
    PoolingLayerForwardPropagation pooling_layer_forward_propagation(
        numb_of_input_images, &pooling_layer);
    
    pooling_layer.forward_propagate(
        input.data(), 
        pooling_input_dimension, 
        static_cast<LayerForwardPropagation*>(&pooling_layer_forward_propagation), 
        switch_train);

    flatten_layer.forward_propagate(
        pooling_layer_forward_propagation.outputs_data, 
        pooling_layer_forward_propagation.outputs_dimensions,
        static_cast<LayerForwardPropagation*>(&flatten_layer_forward_propagation),
        switch_train);

    //test
    Tensor<type, 2> expected_output(2, 8);
    //image 1
    expected_output(0, 0) = type(1);
    expected_output(0, 1) = type(2);
    expected_output(0, 2) = type(3);
    expected_output(0, 3) = type(4);
    expected_output(0, 4) = type(5);
    expected_output(0, 5) = type(6);
    expected_output(0, 6) = type(7);
    expected_output(0, 7) = type(8);
    //image 2
    expected_output.chip(1, 0) = expected_output.chip(0, 0) + type(8);

    TensorMap<Tensor<type, 2>> output(
        flatten_layer_forward_propagation.outputs_data,
        t1d2array<2>(flatten_layer_forward_propagation.outputs_dimensions));
    
    assert_true(is_equal<2>(expected_output, output), LOG);
}

void FlattenPoolingLayerTest::test_pooling_flatten_backward_pass()
{
    cout << "test_pooling_flatten_backward_pass\n";

    const Index numb_of_input_rows = 4;
    const Index numb_of_input_cols = 4;
    const Index numb_of_input_chs = 2;
    const Index numb_of_input_images = 2;

    Tensor<Index, 1> pooling_input_dimension(4);
    pooling_input_dimension[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    pooling_input_dimension[Convolutional4dDimensions::channel_index] = numb_of_input_chs;
    pooling_input_dimension[Convolutional4dDimensions::row_index] = numb_of_input_rows;
    pooling_input_dimension[Convolutional4dDimensions::column_index] = numb_of_input_cols;

    const Index pool_row_size = 2;
    const Index pool_col_size = 2;
    Tensor<Index, 1> pool_dimension(2);
    pool_dimension.setValues({
        pool_row_size,
        pool_col_size
    });

    const Index stride = 2;

    PoolingLayer pooling_layer(pooling_input_dimension, pool_dimension);
    pooling_layer.set_row_stride(stride);
    pooling_layer.set_column_stride(stride);
    pooling_layer.set_pooling_method(PoolingLayer::PoolingMethod::AveragePooling);

    Tensor<Index, 1> flatten_layer_input_dimensions = pooling_layer.get_outputs_dimensions();
    
    FlattenLayer flatten_layer(flatten_layer_input_dimensions);

    FlattenLayerForwardPropagation flatten_layer_forward_propagation(numb_of_input_images, &flatten_layer);

    FlattenLayerBackPropagation flatten_layer_back_propagation(numb_of_input_images, &flatten_layer);
    TensorMap<Tensor<type, 2>> flatten_delta(
        flatten_layer_back_propagation.deltas_data,
        t1d2array<2>(flatten_layer_back_propagation.deltas_dimensions));
    //image 1
    flatten_delta(0, 0) = type(1);
    flatten_delta(0, 1) = type(2);
    flatten_delta(0, 2) = type(3);
    flatten_delta(0, 3) = type(4);
    flatten_delta(0, 4) = type(5);
    flatten_delta(0, 5) = type(6);
    flatten_delta(0, 6) = type(7);
    flatten_delta(0, 7) = type(8);
    //image 2
    flatten_delta.chip(1, 0) = flatten_delta.chip(0, 0) + type(8);

    PoolingLayerBackPropagation pooling_layer_back_propagation(numb_of_input_images, &pooling_layer);

    pooling_layer.calculate_hidden_delta(
        static_cast<LayerForwardPropagation*>(&flatten_layer_forward_propagation),
        static_cast<LayerBackPropagation*>(&flatten_layer_back_propagation),
        static_cast<LayerBackPropagation*>(&pooling_layer_back_propagation));
    
    //test
    Tensor<type, 4> expected_output(2, 2, 2, 2);
    //image 1
    expected_output(0, 0, 0, 0) = type(1);
    expected_output(0, 1, 0, 0) = type(2);
    expected_output(0, 0, 1, 0) = type(3);
    expected_output(0, 1, 1, 0) = type(4);
    expected_output(0, 0, 0, 1) = type(5);
    expected_output(0, 1, 0, 1) = type(6);
    expected_output(0, 0, 1, 1) = type(7);
    expected_output(0, 1, 1, 1) = type(8);
    //image 2
    expected_output.chip(1, Convolutional4dDimensions::sample_index) = expected_output.chip(0, Convolutional4dDimensions::sample_index) + type(8);

    TensorMap<Tensor<type, 4>> output(
        pooling_layer_back_propagation.deltas_data,
        t1d2array<4>(pooling_layer_back_propagation.deltas_dimensions));

    assert_true(is_equal<4>(expected_output, output), LOG);
}

void FlattenPoolingLayerTest::run_test_case()
{
   cout << "Running flatten pooling layer test case...\n";

   test_pooling_flatten_forward_propagate();
   test_pooling_flatten_backward_pass();
   
   cout << "End of flatten pooling layer test case.\n\n";
}


// OpenNN: Open Neural Networks Library.
// Copyright (C) 2005-2021 Artificial Intelligence Techniques, SL.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
;
