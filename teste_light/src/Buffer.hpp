#pragma once



struct Cascade
{
    float splitDepth;
    Mat4 viewProjMatrix;

};
const int SHADOW_MAP_CASCADE_COUNT = 4;
const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT =1024;
Cascade cascades[SHADOW_MAP_CASCADE_COUNT];

class ShadowMapManager 
{
private:

    GLuint shadowMapFBO[SHADOW_MAP_CASCADE_COUNT];
    GLuint shadowMaps[SHADOW_MAP_CASCADE_COUNT];
    bool initialized = false;

public:
    bool Initialize()
    {
        if (initialized) return true;

        // Check for depth texture support
        bool hasDepth24 = strstr((const char*)glGetString(GL_EXTENSIONS), "GL_OES_depth24") != nullptr;
        bool hasDepth32 = strstr((const char*)glGetString(GL_EXTENSIONS), "GL_OES_depth32") != nullptr;
        
        GLenum internalFormat = GL_DEPTH_COMPONENT16;
        GLenum type = GL_UNSIGNED_SHORT;
        
        if (hasDepth32) {
            internalFormat = GL_DEPTH_COMPONENT32F;
            type = GL_FLOAT;
        } else if (hasDepth24) {
            internalFormat = GL_DEPTH_COMPONENT24;
            type = GL_UNSIGNED_INT;
        }

        LogInfo("Using depth format: %d", internalFormat);

        // Generate framebuffers and depth textures for each cascade
        glGenFramebuffers(SHADOW_MAP_CASCADE_COUNT, shadowMapFBO);
        glGenTextures(SHADOW_MAP_CASCADE_COUNT, shadowMaps);

        for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            // Create depth texture
            glBindTexture(GL_TEXTURE_2D, shadowMaps[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 
                        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, type, nullptr);
            
            // Check for OpenGL errors
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                LogError("OpenGL error creating depth texture %d: 0x%x", i, error);
                Cleanup();
                return false;
            }
            
            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMaps[i], 0);
            
            // Check framebuffer attachment error
            error = glGetError();
            if (error != GL_NO_ERROR) {
                LogError("OpenGL error attaching depth texture %d: 0x%x", i, error);
                Cleanup();
                return false;
            }

            // Check framebuffer completeness
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                LogError("Shadow Map Framebuffer %d not complete! Status: 0x%x", i, status);
                switch(status) {
                    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                        LogError("  - Incomplete attachment");
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                        LogError("  - Missing attachment");
                        break;
                    case GL_FRAMEBUFFER_UNSUPPORTED:
                        LogError("  - Unsupported framebuffer format");
                        break;
                    default:
                        LogError("  - Unknown framebuffer error");
                        break;
                }
                Cleanup();
                return false;
            }
        }

        // Restore default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        initialized = true;
        LogInfo("Shadow Map Manager initialized successfully");
        return true;
    }

    void Cleanup()
    {
        if (!initialized) return;

        glDeleteFramebuffers(SHADOW_MAP_CASCADE_COUNT, shadowMapFBO);
        glDeleteTextures(SHADOW_MAP_CASCADE_COUNT, shadowMaps);
        
        memset(shadowMapFBO, 0, sizeof(shadowMapFBO));
        memset(shadowMaps, 0, sizeof(shadowMaps));
        initialized = false;
    }

    void BeginShadowPass(int cascadeIndex)
    {
        if (!initialized || cascadeIndex >= SHADOW_MAP_CASCADE_COUNT) return;

        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO[cascadeIndex]);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Disable color writes
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);


        
        // Enable polygon offset to reduce shadow acne
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);
    }

    void EndShadowPass()
    {
        // Restore default state
        glDisable(GL_POLYGON_OFFSET_FILL);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    void BindShadowMapsForReading(Shader* shader)
    {
        if (!initialized || !shader) return;


        shader->SetFloat("shadowStrength", 1.6f); // começa por aqui (1.25–2.0)
        shader->SetFloat("shadowGamma",    1.5f); // 1.2–2.0: maior = mais escuro
        
        // Bind all shadow maps for reading
        for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            glActiveTexture(GL_TEXTURE2 + i); // Start from texture unit 2
            glBindTexture(GL_TEXTURE_2D, shadowMaps[i]);
            
            // Set shader uniform
            std::string uniformName = "shadowMap[" + std::to_string(i) + "]";
            shader->SetInt(uniformName, 2 + i);
        }
        
        // Set shadow map size uniform
      //  shader->SetFloat("shadowMapSize", (float)SHADOW_WIDTH, (float)SHADOW_HEIGHT);
    }

    void UpdateCascadeUniforms(Shader* shader, const Cascade* cascades)
    {
        if (!shader || !cascades) return;

        for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            std::string matrixUniform = "cascadeshadows[" + std::to_string(i) + "].projViewMatrix";
            std::string distanceUniform = "cascadeshadows[" + std::to_string(i) + "].splitDistance";
            
            shader->SetMatrix4(matrixUniform, cascades[i].viewProjMatrix.x);
            shader->SetFloat(distanceUniform, cascades[i].splitDepth);
        }
    }

    GLuint GetShadowMap(int cascadeIndex) const
    {
        if (cascadeIndex >= 0 && cascadeIndex < SHADOW_MAP_CASCADE_COUNT)
            return shadowMaps[cascadeIndex];
        return 0;
    }

    GLuint GetFramebuffer(int cascadeIndex) const
    {
        if (cascadeIndex >= 0 && cascadeIndex < SHADOW_MAP_CASCADE_COUNT)
            return shadowMapFBO[cascadeIndex];
        return 0;
    }

    bool IsInitialized() const { return initialized; }

 
};


class Quad
{
private:
    GLuint VAO, VBO, EBO;
    bool initialized = false;

    // Vertices para um quad full-screen ou unitário
    float vertices[20] = {
        // positions        // texture coords
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,   // top left
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,   // top right
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,   // bottom right
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f    // bottom left
    };

    unsigned int indices[6] = {
        0, 1, 2,   // first triangle
        2, 3, 0    // second triangle
    };

public:
    bool Initialize()
    {
        if (initialized) return true;

        // Generate buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Bind VAO
        glBindVertexArray(VAO);

        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Upload index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Position attribute (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Texture coordinate attribute (location = 1)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        initialized = true;
        return true;
    }

    void Render()
    {
        if (!initialized) return;

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void Cleanup()
    {
        if (!initialized) return;

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        
        VAO = VBO = EBO = 0;
        initialized = false;
    }

    bool IsInitialized() const { return initialized; }

   
};