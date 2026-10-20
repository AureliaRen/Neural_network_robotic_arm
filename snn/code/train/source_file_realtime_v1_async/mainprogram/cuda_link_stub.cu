__global__ void codex_cuda_link_stub_kernel() {}

void codex_cuda_link_stub() {
    codex_cuda_link_stub_kernel<<<1, 1>>>();
}
