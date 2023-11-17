const aarch64 = 'aarch64';
const x86_64 = 'x86_64';

const archList = [{
    arch: x86_64,
    runner: 'ubuntu-latest',
    docker_tag: 'manylinux2014_x86_64-plugins-deps',
}, {
    arch: aarch64,
    runner: 'linux-arm64',
    docker_tag: 'manylinux2014_aarch64-plugins-deps',
}];

const pluginList = [{
    name: 'wasi_nn',
    bin: 'libwasmedgePluginWasiNN.so',
    testBin: 'wasiNNTests',
    features: [{
        name: 'tensorflowlite',
        options: '-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=TensorFlowLite',
        archList: [aarch64, x86_64],
    }, {
        name: 'wasi_nn-ggml',
        options: '-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=GGML -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS=OFF',
        archList: [aarch64, x86_64],
    }, {
        name: 'wasi_nn-pytorch',
        options: '-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=PyTorch',
        archList: [x86_64],
    }],
}, {
    name: 'wasi_crypto',
    bin: 'libwasmedgePluginWasiCrypto.so',
    testBin: 'wasiCryptoTests',
    options: '-DWASMEDGE_PLUGIN_WASI_CRYPTO=ON',
    archList: [aarch64, x86_64],
}, {
    name: 'wasi_logging',
    bin: 'libwasmedgePluginWasiLogging.so',
    testBin: 'wasiLoggingTests',
    options: '-DWASMEDGE_PLUGIN_WASI_LOGGING=ON',
    archList: [aarch64, x86_64],
}, {
    name: 'wasmedge_process',
    bin: 'libwasmedgePluginWasmEdgeProcess.so',
    testBin: 'wasmedgeProcessTests',
    options: '-DWASMEDGE_PLUGIN_PROCESS=ON',
    archList: [aarch64, x86_64],
}, {
    name: 'wasmedge_tensorflow',
    bin: 'libwasmedgePluginWasmEdgeTensorflow.so',
    testBin: 'wasmedgeTensorflowTests',
    options: '-DWASMEDGE_PLUGIN_TENSORFLOW=ON',
    archList: [aarch64, x86_64],
}, {
    name: 'wasmedge_tensorflowlite',
    bin: 'libwasmedgePluginWasmEdgeTensorflowLite.so',
    testBin: 'wasmedgeTensorflowLiteTests',
    options: '-DWASMEDGE_PLUGIN_TENSORFLOWLITE=ON',
    archList: [aarch64, x86_64],
}, {
    name: 'wasmedge_image',
    bin: 'libwasmedgePluginWasmEdgeImage.so',
    testBin: 'wasmedgeImageTests',
    options: '-DWASMEDGE_PLUGIN_IMAGE=ON',
    archList: [aarch64, x86_64],
}, {
    name: 'wasmedge_opencvmini',
    bin: 'libwasmedgePluginWasmEdgeOpenCVMini.so',
    testBin: 'wasmedgeOpencvminiTests',
    options: '-DWASMEDGE_PLUGIN_OPENCVMINI=ON',
    archList: [aarch64, x86_64],
}, {
    name: 'wasmedge_zlib',
    bin: 'libwasmedgePluginWasmEdgeZlib.so',
    testBin: 'wasmedgeZlibTests',
    options: '-DWASMEDGE_PLUGIN_WASI_CRYPTO=ON -DWASMEDGE_PLUGIN_WASI_LOGGING=ON -DWASMEDGE_PLUGIN_PROCESS=ON -DWASMEDGE_PLUGIN_TENSORFLOW=ON -DWASMEDGE_PLUGIN_TENSORFLOWLITE=ON -DWASMEDGE_PLUGIN_IMAGE=ON -DWASMEDGE_PLUGIN_OPENCVMINI=ON -DWASMEDGE_PLUGIN_ZLIB=ON',
    archList: [aarch64, x86_64],
}, {
    name: 'wasm_bpf',
    bin: 'libwasmedgePluginWasmBpf.so',
    testBin: 'wasmBpfTests',
    options: '-DWASMEDGE_PLUGIN_WASM_BPF=ON -DWASMEDGE_PLUGIN_WASM_BPF_BUILD_LIBBPF_WITH_PKG_CONF=OFF',
    archList: [x86_64],
}, {
    name: 'wasmedge_rustls',
    bin: 'libwasmedge_rustls.so',
    testBin: 'wasmEdgeRUSTLSTests',
    options: '-DWASMEDGE_PLUGIN_RUSTLS=ON',
    archList: [x86_64],
}];

let expandVariables = () => {
    let ret = archList.map((arch) => {
        arch.plugins = pluginList.flatMap((plugin) => {
            if (plugin.hasOwnProperty('features'))
                return plugin.features
                    .filter((ft) => ft.archList.includes(arch.arch))
                    .map((ft) => ({
                        plugin: [plugin.name, ft.name].join('-'),
                        bin: plugin.bin,
                        dir: plugin.name,
                        testBin: plugin.testBin,
                        options: ft.options,
                    }));
            if (plugin.archList.includes(arch.arch))
                return {
                    plugin: plugin.name,
                    bin: plugin.bin,
                    dir: plugin.name,
                    testBin: plugin.testBin,
                    options: plugin.options,
                };
            return [];
        })
        return arch;
    });
    return ret;
};

const core = require('@actions/core');

try {
    output = JSON.stringify(expandVariables());
    core.setOutput('matrix', output);
    console.log(`matrix: ${output}`);
} catch (err) {
    core.setFailed(err.message);
}
