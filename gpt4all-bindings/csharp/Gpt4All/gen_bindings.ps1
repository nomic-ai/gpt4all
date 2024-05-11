ClangSharpPInvokeGenerator `
    --config compatible-codegen multi-file generate-file-scoped-namespaces generate-helper-types exclude-funcs-with-body `
    --file ..\..\..\gpt4all-backend\llmodel_c.h `
    --namespace Gpt4All.Bindings `
    --methodClassName NativeMethods `
    --libraryPath llmodel `
    --include-directory ..\..\..\gpt4all-backend\ `
      C:\ProgramData\mingw64\mingw64\x86_64-w64-mingw32\include\ `
      C:\ProgramData\mingw64\mingw64\lib\gcc\x86_64-w64-mingw32\13.2.0\include\ `
    --with-access-specifier *=Internal `
    --remap `
      void*=IntPtr `
      sbyte*=IntPtr `
      llmodel_prompt_callback=PromptCallback `
      llmodel_response_callback=ResponseCallback `
      llmodel_recalculate_callback=RecalculateCallback `
      llmodel_gpu_device=GPUDevice `
      llmodel_prompt_context=PromptContext `
      llmodel_emb_cancel_callback=EmbCancelCallback `
    --output .\Bindings