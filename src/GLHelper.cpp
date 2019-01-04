//
// Created by Engin Manap on 10.02.2016.
//

#include <random>
#include "GLHelper.h"
#include "GLSLProgram.h"

#include "GameObjects/Light.h"
#include "Material.h"
#include "GameObjects/Model.h"
#include "Utils/GLMUtils.h"

GLuint GLHelper::createShader(GLenum eShaderType, const std::string &strShaderFile) {
    GLuint shader = glCreateShader(eShaderType);
    std::string shaderCode;
    std::ifstream shaderStream(strShaderFile.c_str(), std::ios::in);

    if (shaderStream.is_open()) {
        std::string Line;

        while (getline(shaderStream, Line))
            shaderCode += "\n" + Line;

        shaderStream.close();
    } else {
        std::cerr << strShaderFile.c_str() <<
        " could not be read. Please ensure run directory if you used relative paths." << std::endl;
        getchar();
        return 0;
    }

    const char *shaderCodePtr = shaderCode.c_str();
    glShaderSource(shader, 1, &shaderCodePtr, nullptr);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(shader, infoLogLength, nullptr, strInfoLog);
        const char *strShaderType = nullptr;

        switch (eShaderType) {
            case GL_VERTEX_SHADER:
                strShaderType = "vertex";
                break;

            case GL_GEOMETRY_SHADER:
                strShaderType = "geometry";
                break;

            case GL_FRAGMENT_SHADER:
                strShaderType = "fragment";
                break;
        }

        std::cerr << strShaderType << " type shader " << strShaderFile.c_str() << " could not be compiled:\n" <<
        strInfoLog << std::endl;
        delete[] strInfoLog;

    }
    checkErrors("createShader");
    return shader;
}


GLuint GLHelper::createProgram(const std::vector<GLuint> &shaderList) {
    GLuint program = glCreateProgram();

    for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++) {
        glAttachShader(program, shaderList[iLoop]);
    }

    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, nullptr, strInfoLog);
        std::cerr << "Linking failed: \n" << strInfoLog << std::endl;
        delete[] strInfoLog;
    } else {
        //std::cout << "Program compiled successfully" << std::endl;
    }

    for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++) {
        glDetachShader(program, shaderList[iLoop]);
    }

    checkErrors("createProgram");
    return program;
}


GLuint GLHelper::initializeProgram(const std::string &vertexShaderFile, const std::string &geometryShaderFile, const std::string &fragmentShaderFile,
                                   std::unordered_map<std::string, Uniform *> &uniformMap) {
    GLuint program;
    std::vector<GLuint> shaderList;
    checkErrors("before create shaders");
    shaderList.push_back(createShader(GL_VERTEX_SHADER, vertexShaderFile));
    if(!geometryShaderFile.empty()){
        shaderList.push_back(createShader(GL_GEOMETRY_SHADER, geometryShaderFile));
    }
    shaderList.push_back(createShader(GL_FRAGMENT_SHADER, fragmentShaderFile));


    program = createProgram(shaderList);
    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

    fillUniformMap(program, uniformMap);
    attachGeneralUBOs(program);

    checkErrors("initializeProgram");
    return program;
}

void GLHelper::fillUniformMap(const GLuint program, std::unordered_map<std::string, GLHelper::Uniform *> &uniformMap) const {
    GLint i;
    GLint count;

    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)

    GLint maxLength;

    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    GLchar* name = new GLchar[maxLength]; // variable name in GLSL
    GLsizei length; // name length

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    //std::cout << "Active Uniforms:" << count << std::endl;

    uint32_t uniformLocation;
    for (i = 0; i < count; i++)
    {
        glGetActiveUniform(program, (GLuint)i, maxLength, &length, &size, &type, name);
        uniformLocation = glGetUniformLocation(program, name);

        //std::cout << "Uniform " << i << " Location: " << uniformLocation << " Type: " << type << " Name: " << name << std::endl;
        uniformMap[name] = new Uniform(uniformLocation, name, type, size);
    }

    delete[] name;
}

void GLHelper::attachModelUBO(const uint32_t program) {
    GLuint allModelsAttachPoint = 7;

    int uniformIndex = glGetUniformBlockIndex(program, "ModelInformationBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, allModelsUBOLocation);
        glUniformBlockBinding(program, uniformIndex, allModelsAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, allModelsAttachPoint, allModelsUBOLocation, 0,
                          sizeof(glm::mat4)* NR_MAX_MODELS);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void GLHelper::attachModelIndicesUBO(const uint32_t programID) {
    GLuint allModelIndexesAttachPoint = 8;

    int uniformIndex = glGetUniformBlockIndex(programID, "ModelIndexBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, allModelIndexesUBOLocation);
        glUniformBlockBinding(programID, uniformIndex, allModelIndexesAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, allModelIndexesAttachPoint, allModelIndexesUBOLocation, 0,
                          sizeof(uint32_t) * NR_MAX_MODELS);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void GLHelper::attachMaterialUBO(const uint32_t program, const uint32_t materialID){

    GLuint allMaterialsAttachPoint = 9;

    int uniformIndex = glGetUniformBlockIndex(program, "MaterialInformationBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, allMaterialsUBOLocation);
        glUniformBlockBinding(program, uniformIndex, allMaterialsAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, allMaterialsAttachPoint, allMaterialsUBOLocation, materialID * materialUniformSize,
                          materialUniformSize);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    activeMaterialIndex = materialID;
    checkErrors("attachMaterialUBO");
}

void GLHelper::attachGeneralUBOs(const GLuint program){//Attach the light block to our UBO

    GLuint lightAttachPoint = 0, playerAttachPoint = 1;

    int uniformIndex = glGetUniformBlockIndex(program, "LightSourceBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
        glUniformBlockBinding(program, uniformIndex, lightAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, lightAttachPoint, lightUBOLocation, 0,
                          lightUniformSize * NR_TOTAL_LIGHTS);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    int uniformIndex2 = glGetUniformBlockIndex(program, "PlayerTransformBlock");
    if (uniformIndex2 >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
        glUniformBlockBinding(program, uniformIndex2, playerAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, playerAttachPoint, playerUBOLocation, 0,
                          playerUniformSize);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}


GLHelper::GLHelper(Options *options): options(options) {

    this->screenHeight = options->getScreenHeight();
    this->screenWidth = options->getScreenWidth();
    GLenum rev;
    error = GL_NO_ERROR;
    glewExperimental = GL_TRUE;
    rev = glewInit();

    if (GLEW_OK != rev) {
        std::cout << "GLEW init Error: " << glewGetErrorString(rev) << std::endl;
        exit(1);
    } else {
        std::cout << "GLEW Init: Success!" << std::endl;
    }
    checkErrors("after Context creation");

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits);

    std::cout << "Maximum number of texture image units is " << maxTextureImageUnits << std::endl;
    state = new OpenglState(maxTextureImageUnits);

    lightProjectionMatrixDirectional = glm::ortho(options->getLightOrthogonalProjectionValues().x,
                                                  options->getLightOrthogonalProjectionValues().y,
                                                  options->getLightOrthogonalProjectionValues().z,
                                                  options->getLightOrthogonalProjectionValues().w,
                                                  options->getLightOrthogonalProjectionNearPlane(),
                                                  options->getLightOrthogonalProjectionFarPlane());

    lightProjectionMatrixPoint = glm::perspective(glm::radians(90.0f),
                                                  options->getLightPerspectiveProjectionValues().x,
                                                  options->getLightPerspectiveProjectionValues().y,
                                                  options->getLightPerspectiveProjectionValues().z);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    // Setup
    //glDisable(GL_CULL_FACE);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);

    glEnablei(GL_BLEND, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);



    std::cout << "Rendererer: " << glGetString(GL_RENDERER) << std::endl  // e.g. Intel HD Graphics 3000 OpenGL Engine
              << "GL version: " << glGetString(GL_VERSION) << std::endl;    // e.g. 3.2 INTEL-8.0.61

    std::cout << "Supported GLSL version is "<< (char *) glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    GLint n, i;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    std::cout << "found " << n << " extensions." << std::endl;
    bool isCubeMapArraySupported = false;
    char extensionNameBuffer[100];
    for (i = 0; i < n; i++) {
        sprintf(extensionNameBuffer, "%s", glGetStringi(GL_EXTENSIONS, i));
        if(std::strcmp(extensionNameBuffer, "GL_ARB_texture_cube_map_array") == 0) {
            isCubeMapArraySupported = true;
            break;
        }
    }
    if(!isCubeMapArraySupported) {
        std::cerr << "Cubemap array support is mandatory, exiting.. " << std::endl;
        exit(-1);
    }

    std::cout << "Cubemap array support is present. " << std::endl;

    GLint uniformBufferAlignSize = 0;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignSize);

    if(uniformBufferAlignSize > materialUniformSize) {
        materialUniformSize = uniformBufferAlignSize;
    }


    std::cout << "Uniform alignment size is " << uniformBufferAlignSize << std::endl;

    GLint maxVertexUniformBlockCount = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &maxVertexUniformBlockCount);

    std::cout << "Uniform maxVertexUniformBlockCount size is " << maxVertexUniformBlockCount << std::endl;


    //create the Light Uniform Buffer Object for later usage
    glGenBuffers(1, &lightUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
    std::vector<GLubyte> emptyData(lightUniformSize * NR_TOTAL_LIGHTS, 0);
    glBufferData(GL_UNIFORM_BUFFER, lightUniformSize * NR_TOTAL_LIGHTS, &emptyData[0], GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create player transforms uniform buffer object
    glGenBuffers(1, &playerUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, playerUniformSize, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create material uniform buffer object
    glGenBuffers(1, &allMaterialsUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, allMaterialsUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, materialUniformSize * NR_MAX_MATERIALS, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create model uniform buffer object
    glGenBuffers(1, &allModelsUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, allModelsUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, modelUniformSize * NR_MAX_MODELS, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create model index uniform buffer object
    glGenBuffers(1, &allModelIndexesUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, allModelIndexesUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(uint32_t) * NR_MAX_MODELS, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    //create depth buffer and texture for directional shadow map
    glGenFramebuffers(1, &depthOnlyFrameBufferDirectional);
    glGenTextures(1, &depthMapDirectional);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthMapDirectional);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, options->getShadowMapDirectionalWidth(),
                 options->getShadowMapDirectionalHeight(), NR_TOTAL_LIGHTS, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferDirectional);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMapDirectional, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //create depth buffer and texture for point shadow map
    glGenFramebuffers(1, &depthOnlyFrameBufferPoint);
    // create depth cubemap texture
    glGenTextures(1, &depthCubemapPoint);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, depthCubemapPoint);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, 0, GL_DEPTH_COMPONENT, options->getShadowMapPointWidth(),
                 options->getShadowMapPointHeight(), NR_TOTAL_LIGHTS*6, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //if we clamp to border, then the edges become visible. it should be clamped to edge
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameterfv(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferPoint);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemapPoint, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //create prepass depth
    glGenFramebuffers(1, &depthOnlyFrameBuffer);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create default framebuffer with normal map extraction

    glGenFramebuffers(1, &coloringFrameBuffer);
    glGenTextures(1, &normalMap);
    glBindTexture(GL_TEXTURE_2D, normalMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &diffuseAndSpecularLightedMap);
    glBindTexture(GL_TEXTURE_2D, diffuseAndSpecularLightedMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &ambientMap);
    glBindTexture(GL_TEXTURE_2D, ambientMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, coloringFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, diffuseAndSpecularLightedMap, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ambientMap, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, normalMap, 0);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "coloring frame buffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SSAO Framebuffer
    glGenFramebuffers(1, &ssaoGenerationFrameBuffer);

    glGenTextures(1, &ssaoMap);
    glBindTexture(GL_TEXTURE_2D, ssaoMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoGenerationFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ssaoMap, 0);

    unsigned int attachments2[2] = { GL_NONE, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments2);
    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SSAO frame buffer is not complete: " << fbStatus  << ": " << gluErrorString(fbStatus) << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SSAO Framebuffer

    /**************************** SSAO blur ******************************************/

    // SSAO Framebuffer
    glGenFramebuffers(1, &ssaoBlurFrameBuffer);

    glGenTextures(1, &ssaoBlurredMap);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurredMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ssaoBlurredMap, 0);

    unsigned int attachments3[2] = { GL_NONE, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments3);
    fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SSAO Blur frame buffer is not complete: " << fbStatus  << ": " << gluErrorString(fbStatus) << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /********************** SSAO BLUR END ***********************************************************/

    /****************************** SSAO NOISE **************************************/
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    // generate noise texture
    // ----------------------
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    /****************************** SSAO NOISE **************************************/

    frustumPlanes.resize(6);

    checkErrors("Constructor");
}

GLuint GLHelper::generateBuffer(const GLuint number) {
    GLuint bufferID;
    glGenBuffers(number, &bufferID);
    bufferObjects.push_back(bufferID);

    checkErrors("generateBuffer");
    return bufferID;
}

bool GLHelper::deleteBuffer(const GLuint number, const GLuint bufferID) {
    if (glIsBuffer(bufferID)) {
        glDeleteBuffers(number, &bufferID);
        checkErrors("deleteBuffer");
        return true;
    }
    checkErrors("deleteBuffer");
    return false;
}

bool GLHelper::freeBuffer(const GLuint bufferID) {
    for (unsigned int i = 0; i < bufferObjects.size(); ++i) {
        if (bufferObjects[i] == bufferID) {
            deleteBuffer(1, bufferObjects[i]);
            bufferObjects[i] = bufferObjects[bufferObjects.size() - 1];
            bufferObjects.pop_back();
            checkErrors("freeBuffer");
            return true;
        }
    }
    checkErrors("freeBuffer");
    return false;
}

GLuint GLHelper::generateVAO(const GLuint number) {
    GLuint bufferID;
    glGenVertexArrays(number, &bufferID);
    vertexArrays.push_back(bufferID);
    checkErrors("generateVAO");
    return bufferID;
}

bool GLHelper::deleteVAO(const GLuint number, const GLuint bufferID) {
    if (glIsBuffer(bufferID)) {
        glDeleteVertexArrays(number, &bufferID);
        checkErrors("deleteVAO");
        return true;
    }
    checkErrors("deleteVAO");
    return false;
}

bool GLHelper::freeVAO(const GLuint bufferID) {
    for (unsigned int i = 0; i < vertexArrays.size(); ++i) {
        if (vertexArrays[i] == bufferID) {
            deleteBuffer(1, vertexArrays[i]);
            vertexArrays[i] = vertexArrays[vertexArrays.size() - 1];
            vertexArrays.pop_back();
            checkErrors("freeVAO");
            return true;
        }
    }
    checkErrors("freeVAO");
    return false;
}

void GLHelper::bufferVertexData(const std::vector<glm::vec3> &vertices,
                                const std::vector<glm::mediump_uvec3> &faces,
                                uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer,
                                uint_fast32_t &ebo) {
    // Set up the element array buffer
    ebo = generateBuffer(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(glm::mediump_uvec3), faces.data(), GL_STATIC_DRAW);

    // Set up the vertex attributes
    //FIXME this temp should not be needed, but uint_fast32_t requires a cast. re evaluate using uint32_t
    uint32_t temp;
    glGenVertexArrays(1, &temp);
    glBindVertexArray(temp);
    vao = temp;
    vbo = generateBuffer(1);
    bufferObjects.push_back(vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    //glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, vertexData);
    glVertexAttribPointer(attachPointer, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(attachPointer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    checkErrors("bufferVertexData");
}

void GLHelper::bufferNormalData(const std::vector<glm::vec3> &normals,
                                uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexColor");
}

void GLHelper::bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                                     uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) {
    bufferExtraVertexData(4, GL_FLOAT, extraData.size() * sizeof(glm::vec4), extraData.data(), vao, vbo, attachPointer);
    checkErrors("bufferVertexDataVec4");
}

void GLHelper::bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                                     uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) {
    bufferExtraVertexData(4, GL_UNSIGNED_INT, extraData.size() * sizeof(glm::lowp_uvec4), extraData.data(), vao, vbo,
                          attachPointer);
    checkErrors("bufferVertexDataIVec4");
}

void GLHelper::bufferExtraVertexData(uint_fast32_t elementPerVertexCount, GLenum elementType, uint_fast32_t dataSize,
                                     const void *extraData, uint_fast32_t &vao, uint_fast32_t &vbo,
                                     const uint_fast32_t attachPointer) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, dataSize, extraData, GL_STATIC_DRAW);

    glBindVertexArray(vao);
    switch (elementType) {
        case GL_UNSIGNED_INT:
        case GL_INT:
            glVertexAttribIPointer(attachPointer, elementPerVertexCount, elementType, 0, 0);
            break;
        default:
            glVertexAttribPointer(attachPointer, elementPerVertexCount, elementType, GL_FALSE, 0, 0);
    }

    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferExtraVertexDataInternal");
}

void GLHelper::bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                              uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, textureCoordinates.size() * sizeof(glm::vec2), textureCoordinates.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexTextureCoordinates");
}

void GLHelper::switchRenderToShadowMapDirectional(const unsigned int index) {
    glViewport(0, 0, options->getShadowMapDirectionalWidth(), options->getShadowMapDirectionalHeight());
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferDirectional);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMapDirectional, 0, index);
    glCullFace(GL_FRONT);
    checkErrors("switchRenderToShadowMapDirectional");
}

void GLHelper::switchRenderToShadowMapPoint() {
    glViewport(0, 0, options->getShadowMapPointWidth(), options->getShadowMapPointHeight());
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferPoint);
    glCullFace(GL_FRONT);
    checkErrors("switchRenderToShadowMapPoint");
}

void GLHelper::switchRenderToDepthPrePass() {
    glViewport(0, 0, screenWidth, screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBuffer);
    glCullFace(GL_BACK);
    checkErrors("switchRenderToDepthPrePass");
}

void GLHelper::switchRenderToColoring() {
    glViewport(0, 0, screenWidth, screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, coloringFrameBuffer);

    //we bind shadow map to last texture unit
    state->attach2DTextureArray(depthMapDirectional, maxTextureImageUnits - 1);
    state->attachCubemapArray(depthCubemapPoint, maxTextureImageUnits - 2);
    state->attachTexture(depthMap, maxTextureImageUnits - 3);
    state->attachTexture(noiseTexture, maxTextureImageUnits - 4);
    glCullFace(GL_BACK);
    checkErrors("switchRenderToColoring");
}

void GLHelper::switchRenderToSSAOGeneration() {
    glViewport(0, 0, screenWidth, screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoGenerationFrameBuffer);
    state->attachTexture(depthMap, 1);
    state->attachTexture(normalMap, 2);
    state->attachTexture(noiseTexture, 3);
    glCullFace(GL_BACK);
    checkErrors("switchRenderToSSAOGeneration");
}

void GLHelper::switchRenderToSSAOBlur() {
    glViewport(0, 0, screenWidth, screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFrameBuffer);
    state->attachTexture(ssaoMap, 1);
    glCullFace(GL_BACK);
    checkErrors("switchRenderToSSAOBlur");
}

void GLHelper::switchRenderToCombining(){
    glViewport(0, 0, screenWidth, screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //we combine diffuse+specular lighted with ambient / SSAO
    state->attachTexture(diffuseAndSpecularLightedMap, 1);
    state->attachTexture(ambientMap, 2);
    state->attachTexture(ssaoBlurredMap,3);
    checkErrors("switchRenderToCombining");
}

void GLHelper::render(const GLuint program, const GLuint vao, const GLuint ebo, const GLuint elementCount) {
    if (program == 0) {
        std::cerr << "No program render requested." << std::endl;
        return;
    }
    state->setProgram(program);

    // Set up for a glDrawElements call
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    renderTriangleCount = renderTriangleCount + elementCount;
    glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    checkErrors("render");
}

void GLHelper::renderInstanced(GLuint program, uint_fast32_t VAO, uint_fast32_t EBO, uint_fast32_t triangleCount,
                               uint32_t instanceCount) {
    if (program == 0) {
        std::cerr << "No program render requested." << std::endl;
        return;
    }
    state->setProgram(program);

    // Set up for a glDrawElements call
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    renderTriangleCount = renderTriangleCount + (triangleCount * instanceCount);
    glDrawElementsInstanced(GL_TRIANGLES, triangleCount, GL_UNSIGNED_INT, nullptr, instanceCount);
    glBindVertexArray(0);
    //state->setProgram(0);

    checkErrors("renderInstanced");

}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const glm::mat4 &matrix) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniformMatrix4fv(uniformID, 1, GL_FALSE, glm::value_ptr(matrix));
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformMatrix");
        return true;
    }
}


bool
GLHelper::setUniformArray(const GLuint programID, const GLuint uniformID, const std::vector<glm::mat4> &matrixArray) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        int elementCount = matrixArray.size();
        glUniformMatrix4fv(uniformID, elementCount, GL_FALSE, glm::value_ptr(matrixArray.at(0)));
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformMatrixArray");
        return true;
    }
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const glm::vec3 &vector) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform3fv(uniformID, 1, glm::value_ptr(vector));
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformVector");
        return true;
    }
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const std::vector<glm::vec3> &vectorArray) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform3fv(uniformID, vectorArray.size(), glm::value_ptr(*vectorArray.data()));
                //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformVector");
        return true;
    }
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const float value) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform1f(uniformID, value);
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformFloat");
        return true;
    }
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const int value) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform1i(uniformID, value);
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformInt");
        return true;
    }
}


GLHelper::~GLHelper() {
    for (unsigned int i = 0; i < bufferObjects.size(); ++i) {
        deleteBuffer(1, bufferObjects[i]);
    }

    deleteBuffer(1, lightUBOLocation);
    deleteBuffer(1, playerUBOLocation);
    deleteBuffer(1, allMaterialsUBOLocation);
    deleteBuffer(1, depthMapDirectional);
    deleteBuffer(1, depthCubemapPoint);
    deleteBuffer(1, depthMap);
    glDeleteFramebuffers(1, &depthOnlyFrameBufferDirectional); //maybe we should wrap this up too
    glDeleteFramebuffers(1, &depthOnlyFrameBufferPoint);
    glDeleteFramebuffers(1, &depthOnlyFrameBuffer);
    //state->setProgram(0);
}

void GLHelper::reshape() {
    //reshape actually checks for changes on options->
    this->screenHeight = options->getScreenHeight();
    this->screenWidth = options->getScreenWidth();
    glViewport(0, 0, options->getScreenWidth(), options->getScreenHeight());
    aspect = float(options->getScreenHeight()) / float(options->getScreenWidth());
    perspectiveProjectionMatrix = glm::perspective(options->PI/3.0f, 1.0f / aspect, 0.01f, 10000.0f);
    inverseProjection = glm::inverse(perspectiveProjectionMatrix);
    orthogonalProjectionMatrix = glm::ortho(0.0f, (float) options->getScreenWidth(), 0.0f, (float) options->getScreenHeight());
    checkErrors("reshape");
}

GLuint GLHelper::loadTexture(int height, int width, GLenum format, void *data) {
    GLuint texture;
    glGenTextures(1, &texture);
    state->activateTextureUnit(0);//this is the default working texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    switch (options->getTextureFiltering()) {
        case Options::TextureFilteringModes::NEAREST:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case Options::TextureFilteringModes::BILINEAR:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case Options::TextureFilteringModes::TRILINEAR:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    checkErrors("loadTexture");
    return texture;
}

void GLHelper::attachTexture(unsigned int textureID, unsigned int attachPoint) {
    state->attachTexture(textureID, attachPoint);
    checkErrors("attachTexture");
}

void GLHelper::attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) {
    state->attachCubemap(cubeMapID, attachPoint);
    checkErrors("attachCubeMap");
}

bool GLHelper::deleteTexture(GLuint textureID) {
    if (glIsTexture(textureID)) {
        glDeleteTextures(1, &textureID);
        checkErrors("deleteTexture");
        return true;
    } else {
        checkErrors("deleteTexture");
        return false;
    }
}

GLuint GLHelper::loadCubeMap(int height, int width, void *right, void *left, void *top, void *bottom, void *back,
                             void *front) {
    GLuint cubeMap;
    glGenTextures(1, &cubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, right);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, left);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, top);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bottom);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, back);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, front);
    switch (options->getTextureFiltering()) {
        case Options::TextureFilteringModes::NEAREST:
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case Options::TextureFilteringModes::BILINEAR:
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case Options::TextureFilteringModes::TRILINEAR:
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    checkErrors("loadCubeMap");
    return cubeMap;
}

bool GLHelper::getUniformLocation(const GLuint programID, const std::string &uniformName, GLuint &location) {
    GLint rawLocation = glGetUniformLocation(programID, uniformName.c_str());
    if (!checkErrors("getUniformLocation")) {
        if (rawLocation >= 0) {
            location = rawLocation;
            return true;
        } else {
            std::cerr << "No error found, but uniform[" << uniformName << "] can not be located " << std::endl;
        }
    }
    return false;
}

void GLHelper::createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(Line), nullptr, GL_DYNAMIC_DRAW);
    //glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, vertexData);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 28, nullptr); //position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 28, (void*)12); //color
    glVertexAttribPointer(2, 1, GL_INT,  GL_FALSE, 28, (void*)24); //needsCameraTransform
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    checkErrors("createDebugVAOVBO");
}

/**
 * This method draws lines, but is refreshes whole buffer. In a better world, these values should be
 * loaded with other values, and rendered without the heavy load. but for now, it is faster then the older version.
 *
 * PLEASE NOTE, IF LINE SIZE IS BIGGER THEN ASSIGNED, UNDEFINED BEHAVIOUR
 *
 * @param program - glsl program used to render
 * @param vao     - vao
 * @param vbo     - vbo that the lines will be buffered. It should be
 * @param lines   - line vector
 */
void GLHelper::drawLines(GLSLProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines) {
    state->setProgram(program.getID());
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, lines.size() * sizeof(Line), lines.data());
    program.setUniform("cameraTransformMatrix", perspectiveProjectionMatrix * cameraMatrix);

    renderLineCount = renderLineCount + lines.size();
    glDrawArrays(GL_LINES, 0, lines.size()*2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    checkErrors("drawLines");
}

void GLHelper::setLight(const Light &light, const int i) {
    GLint lightType;
    switch (light.getLightType()) {
        case Light::DIRECTIONAL:
            lightType = 1;
            break;
        case Light::POINT:
            lightType = 2;
            break;
    }

    //std::cout << "light type is " << lightType << std::endl;
    //std::cout << "size is " << sizeof(GLint) << std::endl;
    float farPlane = light.getActiveDistance();
    glm::vec3 attenuation = light.getAttenuation();

    glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize,
                    sizeof(glm::mat4) * 6, light.getShadowMatrices());
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 6,
                    sizeof(glm::mat4), glm::value_ptr(light.getLightSpaceMatrix()));
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7,
                    sizeof(glm::vec3), &light.getPosition());
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + sizeof(glm::vec3),
                    sizeof(GLfloat), &farPlane);
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + sizeof(glm::vec4),
                    sizeof(glm::vec3), &light.getColor());
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + sizeof(glm::vec4) + sizeof(glm::vec3),
                    sizeof(GLint), &lightType);
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + 2 *sizeof(glm::vec4),
                    sizeof(glm::vec3), glm::value_ptr(attenuation));
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + 3 *sizeof(glm::vec4),
                    sizeof(glm::vec3), glm::value_ptr(light.getAmbientColor()));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setLight");
}

void GLHelper::setMaterial(std::shared_ptr<const Material> material) {
    /*
     * this buffer has 2 objects, model has mat4 and then the material below:
     *
    layout (std140) uniform MaterialInformationBlock {
            vec3 ambient;
            float shininess;
            vec3 diffuse;
            int isMap;
    } material;
    */
    float shininess = material->getSpecularExponent();
    uint32_t maps = material->getMaps();

    glBindBuffer(GL_UNIFORM_BUFFER, allMaterialsUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, material->getMaterialIndex() * materialUniformSize,
                    sizeof(glm::vec3), glm::value_ptr(material->getAmbientColor()));
    glBufferSubData(GL_UNIFORM_BUFFER, material->getMaterialIndex() * materialUniformSize + sizeof(glm::vec3),
                    sizeof(GLfloat), &shininess);
    glBufferSubData(GL_UNIFORM_BUFFER, material->getMaterialIndex() * materialUniformSize + sizeof(glm::vec3) + sizeof(GLfloat),
                    sizeof(glm::vec3), glm::value_ptr(material->getDiffuseColor()));
    glBufferSubData(GL_UNIFORM_BUFFER, material->getMaterialIndex() * materialUniformSize + 2 *sizeof(glm::vec3) + sizeof(GLfloat),
                    sizeof(GLint), &maps);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setMaterial");
}

void GLHelper::setModel(const uint32_t modelID, const glm::mat4& worldTransform) {
    glBindBuffer(GL_UNIFORM_BUFFER, allModelsUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, modelID * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(worldTransform));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setModel");
}

void GLHelper::setModelIndexesUBO(std::vector<uint32_t> &modelIndicesList) {
    std::vector<glm::uvec4> temp;
    for (uint32_t i = 0; i < modelIndicesList.size(); ++i) {
        temp.push_back(glm::uvec4(modelIndicesList[i], 0,0,0));
    }
    glBindBuffer(GL_UNIFORM_BUFFER, allModelIndexesUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::uvec4) * modelIndicesList.size(), glm::value_ptr(temp.at(0)));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setModelIndexesUBO");
}

void GLHelper::setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraTransform) {
    this->cameraMatrix = cameraTransform;
    this->cameraPosition= cameraPosition;
    glm::vec3 cameraSpacePosition = glm::vec3(cameraMatrix * glm::vec4(cameraPosition, 1.0));
    glm::mat4 inverseCameraMatrix = glm::inverse(cameraTransform);
    glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, 0 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(cameraMatrix));//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 1 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(perspectiveProjectionMatrix));//never changes
    glm::mat4 viewMatrix = perspectiveProjectionMatrix * cameraMatrix;
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(viewMatrix));//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(inverseProjection));//never changes

    glBufferSubData(GL_UNIFORM_BUFFER, 4 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(inverseCameraMatrix));//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4), sizeof(glm::vec3), glm::value_ptr(cameraPosition));//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4)+ sizeof(glm::vec4), sizeof(glm::vec3), glm::value_ptr(cameraSpacePosition));//changes with camera

    glm::vec2 noiseScale(this->screenWidth / 4, this->screenHeight / 4);
    glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4)+ 2* sizeof(glm::vec4), sizeof(glm::vec2), glm::value_ptr(noiseScale));//never changes
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    calculateFrustumPlanes(cameraMatrix, perspectiveProjectionMatrix, frustumPlanes);
    checkErrors("setPlayerMatrices");
}

void GLHelper::calculateFrustumPlanes(const glm::mat4 &cameraMatrix,
                                      const glm::mat4 &projectionMatrix, std::vector<glm::vec4> &planes) const {
    assert(planes.size() == 6);
    glm::mat4 clipMat;

    for(int i = 0; i < 4; i++) {
        glm::vec4 cameraRow = cameraMatrix[i];
        clipMat[i].x =  cameraRow.x * projectionMatrix[0].x + cameraRow.y * projectionMatrix[1].x +
                        cameraRow.z * projectionMatrix[2].x + cameraRow.w * projectionMatrix[3].x;
        clipMat[i].y =  cameraRow.x * projectionMatrix[0].y + cameraRow.y * projectionMatrix[1].y +
                        cameraRow.z * projectionMatrix[2].y + cameraRow.w * projectionMatrix[3].y;
        clipMat[i].z =  cameraRow.x * projectionMatrix[0].z + cameraRow.y * projectionMatrix[1].z +
                        cameraRow.z * projectionMatrix[2].z + cameraRow.w * projectionMatrix[3].z;
        clipMat[i].w =  cameraRow.x * projectionMatrix[0].w + cameraRow.y * projectionMatrix[1].w +
                        cameraRow.z * projectionMatrix[2].w + cameraRow.w * projectionMatrix[3].w;
    }

    planes[RIGHT].x = clipMat[0].w - clipMat[0].x;
    planes[RIGHT].y = clipMat[1].w - clipMat[1].x;
    planes[RIGHT].z = clipMat[2].w - clipMat[2].x;
    planes[RIGHT].w = clipMat[3].w - clipMat[3].x;
    planes[RIGHT] = glm::normalize(planes[RIGHT]);

    planes[LEFT].x = clipMat[0].w + clipMat[0].x;
    planes[LEFT].y = clipMat[1].w + clipMat[1].x;
    planes[LEFT].z = clipMat[2].w + clipMat[2].x;
    planes[LEFT].w = clipMat[3].w + clipMat[3].x;
    planes[LEFT] = glm::normalize(planes[LEFT]);

    planes[BOTTOM].x = clipMat[0].w + clipMat[0].y;
    planes[BOTTOM].y = clipMat[1].w + clipMat[1].y;
    planes[BOTTOM].z = clipMat[2].w + clipMat[2].y;
    planes[BOTTOM].w = clipMat[3].w + clipMat[3].y;
    planes[BOTTOM] = glm::normalize(planes[BOTTOM]);

    planes[TOP].x = clipMat[0].w - clipMat[0].y;
    planes[TOP].y = clipMat[1].w - clipMat[1].y;
    planes[TOP].z = clipMat[2].w - clipMat[2].y;
    planes[TOP].w = clipMat[3].w - clipMat[3].y;
    planes[TOP] = glm::normalize(planes[TOP]);

    planes[BACK].x = clipMat[0].w - clipMat[0].z;
    planes[BACK].y = clipMat[1].w - clipMat[1].z;
    planes[BACK].z = clipMat[2].w - clipMat[2].z;
    planes[BACK].w = clipMat[3].w - clipMat[3].z;
    planes[BACK] = glm::normalize(planes[BACK]);

    planes[FRONT].x = clipMat[0].w + clipMat[0].z;
    planes[FRONT].y = clipMat[1].w + clipMat[1].z;
    planes[FRONT].z = clipMat[2].w + clipMat[2].z;
    planes[FRONT].w = clipMat[3].w + clipMat[3].z;
    planes[FRONT] = glm::normalize(planes[FRONT]);
}