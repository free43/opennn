//   OpenNN: Open Neural Networks Library
//   www.opennn.net
//
//   C O N V O L U T I O N A L  P O O L I N G   L A Y E R   T E S T   C L A S S
//
//   Artificial Intelligence Techniques SL
//   artelnics@artelnics.com

#include "convolutional_pooling_layer_test.h"

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

ConvolutionalPoolingLayerTest::ConvolutionalPoolingLayerTest() 
{
}


ConvolutionalPoolingLayerTest::~ConvolutionalPoolingLayerTest()
{
}


void ConvolutionalPoolingLayerTest::test_convolution_pooling_forward_propagate()
{
    cout << "test_convolution_pooling_forward_propagate\n";
    
    bool switch_train = true;

    const Index numb_of_input_rows = 5;
    const Index numb_of_input_cols = 5;
    const Index numb_of_input_chs = 2;
    const Index numb_of_input_images = 2;

    Tensor<Index, 1> conv_input_dimension(4);
    conv_input_dimension[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    conv_input_dimension[Convolutional4dDimensions::row_index] = numb_of_input_rows;
    conv_input_dimension[Convolutional4dDimensions::column_index] = numb_of_input_cols;
    conv_input_dimension[Convolutional4dDimensions::channel_index] = numb_of_input_chs;

    const Index numb_of_kernel_rows = 2;
    const Index numb_of_kernel_cols = 2;
    const Index numb_of_kernels = 2;

    Tensor<Index, 1> kernel_dimension(4);
    kernel_dimension[Kernel4dDimensions::row_index] = numb_of_kernel_rows;
    kernel_dimension[Kernel4dDimensions::column_index] = numb_of_kernel_cols;
    kernel_dimension[Kernel4dDimensions::channel_index] = numb_of_input_chs;
    kernel_dimension[Kernel4dDimensions::kernel_index] = numb_of_kernels;

    ConvolutionalLayer conv_layer(
        conv_input_dimension, 
        kernel_dimension);
    conv_layer.set_activation_function(ConvolutionalLayer::ActivationFunction::RectifiedLinear);
    
    Tensor<type, 4> kernels(t1d2array<4>(kernel_dimension));
    kernels(0, 0, 0, 0) = type(1);
    kernels(1, 0, 0, 0) = type(2);
    kernels(0, 1, 0, 0) = type(3);
    kernels(1, 1, 0, 0) = type(4);
    kernels(0, 0, 1, 0) = type(5);
    kernels(1, 0, 1, 0) = type(6);
    kernels(0, 1, 1, 0) = type(7);
    kernels(1, 1, 1, 0) = type(8);

    //negate first kernel
    kernels.chip(1, Kernel4dDimensions::kernel_index) = -kernels.chip(0, Kernel4dDimensions::kernel_index);

    Tensor<type, 1> bias(numb_of_kernels);
    bias.setConstant(type(0));

    conv_layer.set_synaptic_weights(kernels);
    conv_layer.set_biases(bias);

    Tensor<Index, 1> pooling_layer_input_dimension = conv_layer.get_outputs_dimensions();

    const Index pooling_layer_row_size = 2;
    const Index pooling_layer_col_size = 2;
    const Index stride = 2;
    
    Tensor<Index, 1> pool_dimension(2);
    pool_dimension.setValues({
        pooling_layer_row_size,
        pooling_layer_col_size,
    });

    PoolingLayer pooling_layer(pooling_layer_input_dimension, pool_dimension);
    pooling_layer.set_row_stride(stride);
    pooling_layer.set_column_stride(stride);
    pooling_layer.set_pooling_method(PoolingLayer::PoolingMethod::MaxPooling);

    ConvolutionalLayerForwardPropagation conv_forward_propagation(numb_of_input_images, &conv_layer);
    PoolingLayerForwardPropagation pooling_layer_forward_propagation(numb_of_input_images, &pooling_layer);

    Tensor<type, 4> conv_input(t1d2array<4>(conv_input_dimension));
    conv_input.setConstant(type(1));

    conv_layer.forward_propagate(
        conv_input.data(), 
        conv_input_dimension, 
        static_cast<LayerForwardPropagation*>(&conv_forward_propagation), 
        switch_train);
    pooling_layer.forward_propagate(
        conv_forward_propagation.outputs_data, 
        conv_forward_propagation.outputs_dimensions, 
        static_cast<LayerForwardPropagation*>(&pooling_layer_forward_propagation), 
        switch_train);

    TensorMap<Tensor<type, 4>> output(
        pooling_layer_forward_propagation.outputs_data, 
        t1d2array<4>(pooling_layer_forward_propagation.outputs_dimensions));
    //test
    Tensor<type, 4> expected_output(2, 2, 2, 2);
    expected_output.setZero();
    expected_output.chip(0, Convolutional4dDimensions::channel_index).setConstant(type(36));

    assert_true(is_equal<4>(expected_output, output), LOG);
}

void ConvolutionalPoolingLayerTest::test_pooling_convolution_forward_propagate()
{
    cout << "test_pooling_convolution_forward_propagate\n";

    bool switch_train = true;

    const Index numb_of_input_rows = 6;
    const Index numb_of_input_cols = 6;
    const Index numb_of_input_chs = 2;
    const Index numb_of_input_images = 2;

    Tensor<Index, 1> pooling_input_dimension(4);
    pooling_input_dimension[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    pooling_input_dimension[Convolutional4dDimensions::row_index] = numb_of_input_rows;
    pooling_input_dimension[Convolutional4dDimensions::column_index] = numb_of_input_cols;
    pooling_input_dimension[Convolutional4dDimensions::channel_index] = numb_of_input_chs;


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
    pooling_layer.set_pooling_method(PoolingLayer::PoolingMethod::MaxPooling);

    const Index numb_of_kernel_rows = 2;
    const Index numb_of_kernel_cols = 2;
    const Index numb_of_kernels = 1;

    Tensor<Index, 1> kernel_dimension(4);
    kernel_dimension[Kernel4dDimensions::row_index] = numb_of_kernel_rows;
    kernel_dimension[Kernel4dDimensions::column_index] = numb_of_kernel_cols;
    kernel_dimension[Kernel4dDimensions::channel_index] = numb_of_input_chs;
    kernel_dimension[Kernel4dDimensions::kernel_index] = numb_of_kernels;


    Tensor<type, 4> kernel(t1d2array<4>(kernel_dimension));
    kernel.setConstant(type(1));

    Tensor<type, 1> bias(numb_of_kernels);
    bias.setConstant(type(0));

    Tensor<Index, 1> conv_input_dimension = pooling_layer.get_outputs_dimensions();

    ConvolutionalLayer conv_layer(conv_input_dimension, kernel_dimension);
    conv_layer.set_synaptic_weights(kernel);
    conv_layer.set_biases(bias);
    conv_layer.set_activation_function(ConvolutionalLayer::ActivationFunction::RectifiedLinear);

    PoolingLayerForwardPropagation pooling_layer_forward_propagation(numb_of_input_images, &pooling_layer);
    ConvolutionalLayerForwardPropagation conv_layer_forward_propagation(numb_of_input_images, &conv_layer);

    Tensor<type, 4> input(t1d2array<4>(pooling_input_dimension));
    input(0, 0, 0, 0) = type(1);
    input(0, 1, 0, 0) = type(2);
    input(0, 0, 1, 0) = type(3);
    input(0, 1, 1, 0) = type(4);
    input(0, 0, 2, 0) = type(5);
    input(0, 1, 2, 0) = type(6);
    input(0, 0, 3, 0) = type(7);
    input(0, 1, 3, 0) = type(8);
    input(0, 0, 4, 0) = type(9);
    input(0, 1, 4, 0) = type(10);
    input(0, 0, 5, 0) = type(11);
    input(0, 1, 5, 0) = type(12);
    input.chip(1, Convolutional4dDimensions::row_index) = input.chip(0, Convolutional4dDimensions::row_index) + type(12);
    input.chip(2, Convolutional4dDimensions::row_index) = input.chip(1, Convolutional4dDimensions::row_index) + type(12);
    input.chip(3, Convolutional4dDimensions::row_index) = input.chip(2, Convolutional4dDimensions::row_index) + type(12);
    input.chip(4, Convolutional4dDimensions::row_index) = input.chip(3, Convolutional4dDimensions::row_index) + type(12);
    input.chip(5, Convolutional4dDimensions::row_index) = input.chip(4, Convolutional4dDimensions::row_index) + type(12);
    
    input.chip(1, Convolutional4dDimensions::sample_index) = input.chip(0, Convolutional4dDimensions::sample_index);

    pooling_layer.forward_propagate(
        input.data(), 
        pooling_input_dimension, 
        static_cast<LayerForwardPropagation*>(&pooling_layer_forward_propagation), 
        switch_train);
    conv_layer.forward_propagate(
        pooling_layer_forward_propagation.outputs_data,
        pooling_layer.get_outputs_dimensions(),
        static_cast<LayerForwardPropagation*>(&conv_layer_forward_propagation),
        switch_train
    );

    //test
    DSizes<Index, 4> expected_output_dim{};
    expected_output_dim[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    expected_output_dim[Convolutional4dDimensions::channel_index] = numb_of_kernels;
    expected_output_dim[Convolutional4dDimensions::row_index] = 2;
    expected_output_dim[Convolutional4dDimensions::column_index] = 2;

    Tensor<type, 4> expected_output(expected_output_dim);
    expected_output(0, 0, 0, 0) = type(236);
    expected_output(0, 0, 1, 0) = type(268);
    expected_output(0, 0, 0, 1) = type(428);
    expected_output(0, 0, 1, 1) = type(460);
    expected_output.chip(1, Convolutional4dDimensions::sample_index) = expected_output.chip(0, Convolutional4dDimensions::sample_index);

    TensorMap<Tensor<type, 4>> output(
        conv_layer_forward_propagation.outputs_data,
        t1d2array<4>(conv_layer_forward_propagation.outputs_dimensions));

    assert_true(is_equal<4>(expected_output, output), LOG);
}

void ConvolutionalPoolingLayerTest::test_convolution_pooling_backward_pass()
{
    cout << "test_convolution_pooling_backward_pass\n";
    const Index numb_of_input_rows = 5;
    const Index numb_of_input_cols = 5;
    const Index numb_of_input_chs = 2;
    const Index numb_of_input_images = 2;

    Tensor<Index, 1> conv_input_dimension(4);
    conv_input_dimension[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    conv_input_dimension[Convolutional4dDimensions::channel_index] = numb_of_input_chs;
    conv_input_dimension[Convolutional4dDimensions::row_index] = numb_of_input_rows;
    conv_input_dimension[Convolutional4dDimensions::column_index] = numb_of_input_cols;

    const Index numb_of_kernel_rows = 2;
    const Index numb_of_kernel_cols = 2;
    const Index numb_of_kernels = 2;

    Tensor<Index, 1> kernel_dimension(4);
    kernel_dimension[Kernel4dDimensions::kernel_index] = numb_of_kernels;
    kernel_dimension[Kernel4dDimensions::row_index] = numb_of_kernel_rows;
    kernel_dimension[Kernel4dDimensions::column_index] = numb_of_kernel_cols;
    kernel_dimension[Kernel4dDimensions::channel_index] = numb_of_input_chs;

    ConvolutionalLayer conv_layer(
        conv_input_dimension, 
        kernel_dimension);
    conv_layer.set_activation_function(ConvolutionalLayer::ActivationFunction::RectifiedLinear);

    Tensor<Index, 1> pooling_layer_input_dimension = conv_layer.get_outputs_dimensions();

    const Index pooling_layer_row_size = 2;
    const Index pooling_layer_col_size = 2;
    const Index stride = 2;
    
    Tensor<Index, 1> pool_dimension(2);
    pool_dimension.setValues({
        pooling_layer_row_size,
        pooling_layer_col_size,
    });

    PoolingLayer pooling_layer(pooling_layer_input_dimension, pool_dimension);
    pooling_layer.set_row_stride(stride);
    pooling_layer.set_column_stride(stride);
    pooling_layer.set_pooling_method(PoolingLayer::PoolingMethod::MaxPooling);

    PoolingLayerForwardPropagation pooling_layer_forward_propagation(
        numb_of_input_images,
        &pooling_layer
    );

    Tensor<type, 4> pooling_layer_inputs{t1d2array<4>(pooling_layer.get_input_variables_dimensions())};
    pooling_layer_inputs.setConstant(static_cast<type>(-2));
    //image 1
    pooling_layer_inputs(0, 0, 0, 0) = static_cast<type>(-1);
    pooling_layer_inputs(0, 1, 1, 0) = static_cast<type>(-1);
    pooling_layer_inputs(0, 0, 3, 1) = static_cast<type>(-1);
    pooling_layer_inputs(0, 1, 2, 0) = static_cast<type>(-1);
    pooling_layer_inputs(0, 0, 0, 3) = static_cast<type>(-1);
    pooling_layer_inputs(0, 1, 1, 2) = static_cast<type>(5);
    pooling_layer_inputs(0, 0, 3, 2) = static_cast<type>(5);
    pooling_layer_inputs(0, 1, 2, 3) = static_cast<type>(5);
    //image 2
    pooling_layer_inputs(1, 0, 1, 1) = static_cast<type>(5);
    pooling_layer_inputs(1, 1, 0, 1) = static_cast<type>(5);
    pooling_layer_inputs(1, 0, 2, 0) = static_cast<type>(-1);
    pooling_layer_inputs(1, 1, 2, 1) = static_cast<type>(-1);
    pooling_layer_inputs(1, 0, 0, 2) = static_cast<type>(-1);
    pooling_layer_inputs(1, 1, 1, 3) = static_cast<type>(-1);
    pooling_layer_inputs(1, 0, 3, 3) = static_cast<type>(-1);
    pooling_layer_inputs(1, 1, 2, 2) = static_cast<type>(-1);

    TensorMap<Tensor<type, 4>> outputs_m{
        pooling_layer_forward_propagation.outputs_data, 
        t1d2array<4>(pooling_layer_forward_propagation.outputs_dimensions)
    };

    pooling_layer.calculate_max_pooling_outputs(pooling_layer_inputs, outputs_m, pooling_layer_forward_propagation.pimpl.get());

    PoolingLayerBackPropagation pooling_layer_back_propagation(
        numb_of_input_images,
        &pooling_layer
    );
    TensorMap<Tensor<type, 4>> next_delta(
        pooling_layer_back_propagation.deltas_data, 
        t1d2array<4>(pooling_layer_back_propagation.deltas_dimensions));
    next_delta(0, 0, 0, 0) = type(1);
    next_delta(0, 1, 0, 0) = type(2);
    next_delta(0, 0, 1, 0) = type(3);
    next_delta(0, 1, 1, 0) = type(4);
    next_delta(0, 0, 0, 1) = type(5);
    next_delta(0, 1, 0, 1) = type(6);
    next_delta(0, 0, 1, 1) = type(7);
    next_delta(0, 1, 1, 1) = type(8);
    next_delta.chip(1, Convolutional4dDimensions::sample_index) = next_delta.chip(0, Convolutional4dDimensions::sample_index) + type(8);
    
    ConvolutionalLayerBackPropagation conv_layer_back_propagation(numb_of_input_images, &conv_layer);

    conv_layer.calculate_hidden_delta(
        static_cast<LayerForwardPropagation*>(&pooling_layer_forward_propagation), 
        static_cast<LayerBackPropagation*>(&pooling_layer_back_propagation),
        static_cast<LayerBackPropagation*>(&conv_layer_back_propagation));

    //test
    DSizes<Index, 4> expected_output_dim{};
    expected_output_dim[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    expected_output_dim[Convolutional4dDimensions::channel_index] = 2;
    expected_output_dim[Convolutional4dDimensions::row_index] = 4;
    expected_output_dim[Convolutional4dDimensions::column_index] = 4;
    Tensor<type, 4> expected_output(expected_output_dim);
    expected_output.setZero();
    //image 1
    expected_output(0, 0, 0, 0) = type(1);
    expected_output(0, 1, 1, 0) = type(2);
    expected_output(0, 0, 3, 1) = type(3);
    expected_output(0, 1, 2, 0) = type(4);
    expected_output(0, 0, 0, 3) = type(5);
    expected_output(0, 1, 1, 2) = type(6);
    expected_output(0, 0, 3, 2) = type(7);
    expected_output(0, 1, 2, 3) = type(8);
    //image 2
    expected_output(1, 0, 1, 1) = type(9);
    expected_output(1, 1, 0, 1) = type(10);
    expected_output(1, 0, 2, 0) = type(11);
    expected_output(1, 1, 2, 1) = type(12);
    expected_output(1, 0, 0, 2) = type(13);
    expected_output(1, 1, 1, 3) = type(14);
    expected_output(1, 0, 3, 3) = type(15);
    expected_output(1, 1, 2, 2) = type(16);

    TensorMap<Tensor<type, 4>> output(
        conv_layer_back_propagation.deltas_data,
        t1d2array<4>(conv_layer_back_propagation.deltas_dimensions));


    assert_true(is_equal<4>(expected_output, output), LOG);
}

void ConvolutionalPoolingLayerTest::test_pooling_convolution_backward_pass()
{
    cout << "test_pooling_convolution_backward_pass\n";

    const Index numb_of_input_rows = 6;
    const Index numb_of_input_cols = 6;
    const Index numb_of_input_chs = 2;
    const Index numb_of_input_images = 2;

    Tensor<Index, 1> pooling_input_dimension(4);
    pooling_input_dimension[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    pooling_input_dimension[Convolutional4dDimensions::row_index] = numb_of_input_rows;
    pooling_input_dimension[Convolutional4dDimensions::column_index] = numb_of_input_cols;
    pooling_input_dimension[Convolutional4dDimensions::channel_index] = numb_of_input_chs;

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
    pooling_layer.set_pooling_method(PoolingLayer::PoolingMethod::MaxPooling);

    const Index numb_of_kernel_rows = 2;
    const Index numb_of_kernel_cols = 2;
    const Index numb_of_kernels = 1;

    Tensor<Index, 1> kernel_dimension(4);
    kernel_dimension[Kernel4dDimensions::kernel_index] = numb_of_kernels;
    kernel_dimension[Kernel4dDimensions::row_index] = numb_of_kernel_rows;
    kernel_dimension[Kernel4dDimensions::column_index] = numb_of_kernel_cols;
    kernel_dimension[Kernel4dDimensions::channel_index] = numb_of_input_chs;

    Tensor<type, 4> kernel(t1d2array<4>(kernel_dimension));
    kernel(0, 0, 0, 0) = type(1);
    kernel(1, 0, 0, 0) = type(2);
    kernel(0, 1, 0, 0) = type(3);
    kernel(1, 1, 0, 0) = type(4);
    kernel(0, 0, 1, 0) = type(5);
    kernel(1, 0, 1, 0) = type(6);
    kernel(0, 1, 1, 0) = type(7);
    kernel(1, 1, 1, 0) = type(8);
    
    Tensor<Index, 1> conv_input_dimension = pooling_layer.get_outputs_dimensions();

    ConvolutionalLayer conv_layer(conv_input_dimension, kernel_dimension);
    conv_layer.set_synaptic_weights(kernel);
    conv_layer.set_activation_function(ConvolutionalLayer::ActivationFunction::RectifiedLinear);
    
    ConvolutionalLayerForwardPropagation conv_layer_forward_propagation(numb_of_input_images, &conv_layer);
    conv_layer_forward_propagation.activations_derivatives.setZero();
    conv_layer_forward_propagation.activations_derivatives(0, 0, 0, 0) = type(1);
    conv_layer_forward_propagation.activations_derivatives(0, 0, 1, 1) = type(1);
    conv_layer_forward_propagation.activations_derivatives.chip(1, Convolutional4dDimensions::sample_index) = 
        conv_layer_forward_propagation.activations_derivatives.chip(0, Convolutional4dDimensions::sample_index);

    ConvolutionalLayerBackPropagation conv_layer_back_propagation(numb_of_input_images, &conv_layer);
    TensorMap<Tensor<type, 4>> delta(
        conv_layer_back_propagation.deltas_data,
        t1d2array<4>(conv_layer_back_propagation.deltas_dimensions));
    //image 1
    delta(0, 0, 0, 0) = type(1);
    delta(0, 0, 1, 0) = type(2);
    delta(0, 0, 0, 1) = type(3);
    delta(0, 0, 1, 1) = type(4);
    //image 2
    delta(1, 0, 0, 0) = type(5);
    delta(1, 0, 1, 0) = type(6);
    delta(1, 0, 0, 1) = type(7);
    delta(1, 0, 1, 1) = type(8);

    PoolingLayerBackPropagation pooling_layer_back_propagation(
        numb_of_input_images,
        &pooling_layer);
    pooling_layer.calculate_hidden_delta(
        static_cast<LayerForwardPropagation*>(&conv_layer_forward_propagation),
        static_cast<LayerBackPropagation*>(&conv_layer_back_propagation),
        static_cast<LayerBackPropagation*>(&pooling_layer_back_propagation));

    //test
    DSizes<Index, 4> output_dim{};
    output_dim[Convolutional4dDimensions::sample_index] = numb_of_input_images;
    output_dim[Convolutional4dDimensions::channel_index] = 2;
    output_dim[Convolutional4dDimensions::row_index] = 3;
    output_dim[Convolutional4dDimensions::column_index] = 3;
    Tensor<type, 4> expected_output(output_dim);
    //image 1
    expected_output(0, 0, 0, 0) = type(1);
    expected_output(0, 1, 0, 0) = type(2);
    expected_output(0, 0, 1, 0) = type(3);
    expected_output(0, 1, 1, 0) = type(4);
    expected_output(0, 0, 2, 0) = type(0);
    expected_output(0, 1, 2, 0) = type(0);
    expected_output(0, 0, 0, 1) = type(5);
    expected_output(0, 1, 0, 1) = type(6);
    expected_output(0, 0, 1, 1) = type(11);
    expected_output(0, 1, 1, 1) = type(16);
    expected_output(0, 0, 2, 1) = type(12);
    expected_output(0, 1, 2, 1) = type(16);
    expected_output(0, 0, 0, 2) = type(0);
    expected_output(0, 1, 0, 2) = type(0);
    expected_output(0, 0, 1, 2) = type(20);
    expected_output(0, 1, 1, 2) = type(24);
    expected_output(0, 0, 2, 2) = type(28);
    expected_output(0, 1, 2, 2) = type(32);
    //image 2
    expected_output(1, 0, 0, 0) = type(5);
    expected_output(1, 1, 0, 0) = type(10);
    expected_output(1, 0, 1, 0) = type(15);
    expected_output(1, 1, 1, 0) = type(20);
    expected_output(1, 0, 2, 0) = type(0);
    expected_output(1, 1, 2, 0) = type(0);
    expected_output(1, 0, 0, 1) = type(25);
    expected_output(1, 1, 0, 1) = type(30);
    expected_output(1, 0, 1, 1) = type(43);
    expected_output(1, 1, 1, 1) = type(56);
    expected_output(1, 0, 2, 1) = type(24);
    expected_output(1, 1, 2, 1) = type(32);
    expected_output(1, 0, 0, 2) = type(0);
    expected_output(1, 1, 0, 2) = type(0);
    expected_output(1, 0, 1, 2) = type(40);
    expected_output(1, 1, 1, 2) = type(48);
    expected_output(1, 0, 2, 2) = type(56);
    expected_output(1, 1, 2, 2) = type(64);

    TensorMap<Tensor<type, 4>> output(
        pooling_layer_back_propagation.deltas_data,
        t1d2array<4>(pooling_layer_back_propagation.deltas_dimensions));


    assert_true(is_equal<4>(expected_output, output), LOG);
}



void ConvolutionalPoolingLayerTest::run_test_case()
{
   cout << "Running convolutional pooling layer test case...\n";
   test_convolution_pooling_forward_propagate();
   test_pooling_convolution_forward_propagate();
   test_convolution_pooling_backward_pass();
   test_pooling_convolution_backward_pass();

   cout << "End of convolutional layer test case.\n\n";
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
