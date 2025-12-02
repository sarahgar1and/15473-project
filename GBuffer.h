#pragma once
#include <GL/glew.h>
#include <iostream>

class GBuffer {
public:
    enum GBUFFER_TEXTURE_TYPE {
        GBUFFER_TEXTURE_POSITION,
        GBUFFER_TEXTURE_NORMAL,
        GBUFFER_TEXTURE_ALBEDO_SPEC, // diffuse (rgb) + shininess (a)
        GBUFFER_TEXTURE_SPECULAR, // specular color (rgb)
        GBUFFER_TEXTURE_COUNT
    };

    GLuint fbo = 0;
    GLuint textures[GBUFFER_TEXTURE_COUNT];
    GLuint depthTexture = 0;
    int width, height;

    GBuffer(int w, int h) : width(w), height(h)
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(GBUFFER_TEXTURE_COUNT, textures);

        // ===============================
        // Position buffer (RGBA16F)
        // ===============================
        CreateTexture(
            textures[GBUFFER_TEXTURE_POSITION],
            GL_RGBA16F, GL_RGBA, GL_FLOAT
        );
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_POSITION], 0
        );

        // ===============================
        // Normal buffer (RGBA16F)
        // ===============================
        CreateTexture(
            textures[GBUFFER_TEXTURE_NORMAL],
            GL_RGBA16F, GL_RGBA, GL_FLOAT
        );
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
            GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_NORMAL], 0
        );

        // ===============================
        // Albedo.rgb + Shininess.a (RGBA16F for precision)
        // ===============================
        CreateTexture(
            textures[GBUFFER_TEXTURE_ALBEDO_SPEC],
            GL_RGBA16F, GL_RGBA, GL_FLOAT
        );
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
            GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_ALBEDO_SPEC], 0
        );

        // ===============================
        // Specular color (RGB16F for precision)
        // ===============================
        CreateTexture(
            textures[GBUFFER_TEXTURE_SPECULAR],
            GL_RGB16F, GL_RGB, GL_FLOAT
        );
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
            GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_SPECULAR], 0
        );

        // ===============================
        // Depth buffer (Renderbuffer)
        // ===============================
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(
            GL_TEXTURE_2D, 0,
            GL_DEPTH_COMPONENT24,
            width, height,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, depthTexture, 0
        );

        // Tell OpenGL which color attachments to draw to
        GLenum attachments[4] = {
            GL_COLOR_ATTACHMENT0, // gPosition
            GL_COLOR_ATTACHMENT1, // gNormal
            GL_COLOR_ATTACHMENT2, // gAlbedoSpec
            GL_COLOR_ATTACHMENT3  // gSpecular
        };
        glDrawBuffers(4, attachments);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "GBuffer incomplete!\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ======================
    // Bind for geometry pass
    // ======================
    void BindForWriting() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
    }

    // ======================
    // Bind for lighting pass
    // ======================
    void BindForReading()
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_POSITION]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_NORMAL]);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_ALBEDO_SPEC]);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_SPECULAR]);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
    }

    // Bind all G-buffer textures to the lighting shader
    void BindTextures(GLuint shaderID)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_POSITION]);
        glUniform1i(glGetUniformLocation(shaderID, "gPosition"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_NORMAL]);
        glUniform1i(glGetUniformLocation(shaderID, "gNormal"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_ALBEDO_SPEC]);
        glUniform1i(glGetUniformLocation(shaderID, "gAlbedoSpec"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_SPECULAR]);
        glUniform1i(glGetUniformLocation(shaderID, "gSpecular"), 3);
    }




private:
    void CreateTexture(GLuint tex, GLenum internalFormat, GLenum format, GLenum type)
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(
            GL_TEXTURE_2D, 0,
            internalFormat,
            width, height,
            0,
            format, type,
            nullptr
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
};
