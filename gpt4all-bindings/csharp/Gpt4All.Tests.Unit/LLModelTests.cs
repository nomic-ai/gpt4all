using Microsoft.Extensions.Logging;
using Moq;

namespace Gpt4All.Tests.Unit;

public class LLModelTests
{
    [Fact]
    public void Generate_ShouldCallExpected()
    {
        // Arrange
        var expected = "Expected response";
        var modelPath = "path/to/model";
        var mockLogger = new Mock<ILogger<LLModel>>();
        var mockIOWrapper = new Mock<IIOWrapper>();
        var mockNativeLib = new Mock<INativeInteropWrapper>();

        mockIOWrapper.Setup(x => x.FileExists(modelPath)).Returns(true);
        mockNativeLib.Setup(x => x.ModelDestroy(It.IsAny<nint>()));
        mockNativeLib.Setup(x => x.ModelCreate(It.IsAny<string>(), It.IsAny<string>())).Returns(0xdead);
        mockNativeLib.Setup(x => x.LoadModel(It.IsAny<nint>(), It.IsAny<string>(), It.IsAny<int>(), It.IsAny<int>())).Returns(true);
        mockNativeLib.Setup(x => x.Prompt(
            It.IsAny<nint>(),
            It.IsAny<string>(),
            It.IsAny<string>(),
            It.IsAny<PromptCallback>(),
            It.IsAny<ResponseCallback>(),
            It.IsAny<RecalculateCallback>(),
            It.IsAny<Models.PromptContext>(),
            It.IsAny<bool>(),
            It.IsAny<string?>()
        ))
            .Returns(new Models.PromptContext())
            .Callback((IntPtr _, string _, string _, PromptCallback _, ResponseCallback responseCallback, RecalculateCallback _, Models.PromptContext _, bool _, string? _) =>
            {
                responseCallback.Invoke(1, expected);
            });

        // Act
        var model = new LLModel(modelPath, mockNativeLib.Object, mockLogger.Object, mockIOWrapper.Object);
        var response = model.Generate("Prompt", "Prompt Tempalte");
        model.Dispose();

        // Assert
        Assert.Equal(expected, response.Response);
        mockNativeLib.VerifyAll();
        mockIOWrapper.VerifyAll();
    }

    [Fact]
    public void Embed_ShouldCallExpected()
    {
        // Arrange
        var expected = new float[] { 1, 2, 3 };
        var modelPath = "path/to/model";
        var mockLogger = new Mock<ILogger<LLModel>>();
        var mockIOWrapper = new Mock<IIOWrapper>();
        var mockNativeLib = new Mock<INativeInteropWrapper>();
        long outValue = 0;

        mockIOWrapper.Setup(x => x.FileExists(modelPath)).Returns(true);
        mockNativeLib.Setup(x => x.ModelDestroy(It.IsAny<nint>()));
        mockNativeLib.Setup(x => x.ModelCreate(It.IsAny<string>(), It.IsAny<string>())).Returns(0xdead);
        mockNativeLib.Setup(x => x.LoadModel(It.IsAny<nint>(), It.IsAny<string>(), It.IsAny<int>(), It.IsAny<int>())).Returns(true);
        mockNativeLib.Setup(x => x.Embed(
            It.IsAny<nint>(),
            It.IsAny<string[]>(),
            It.IsAny<string?>(),
            It.IsAny<int>(),
            It.IsAny<bool>(),
            It.IsAny<bool>(),
            It.IsAny<EmbCancelCallback>(),
            out outValue
        )).Returns(expected);

        // Act
        var model = new LLModel(modelPath, mockNativeLib.Object, mockLogger.Object, mockIOWrapper.Object);
        var embeddingsResponse = model.Embed(["Text to embed"]);
        model.Dispose();

        // Assert
        Assert.Equal(expected, embeddingsResponse.Embeddings);
        mockNativeLib.VerifyAll();
        mockIOWrapper.VerifyAll();
    }
}
