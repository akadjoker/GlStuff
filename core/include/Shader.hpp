#pragma once


#include "Config.hpp"
#include "Utils.hpp"


class CORE_PUBLIC Shader 
{
public:
    Shader();
    virtual ~Shader();


    bool Create(const char* vShaderCode, const char* fShaderCode);

    bool Load(const char* vShaderCode, const char* fShaderCode);

    bool operator==(const Shader& other) const
    {
        return m_program == other.m_program;
    }
    bool operator!=(const Shader& other) const
    {
        return m_program != other.m_program;
    }


    u32 GetID() { return m_program; }

    void Use(bool set = true) const;


    void SetInt(const std::string& name, int value);

    void SetMatrix4(const std::string& name, const float* value);
    void SetMatrix3(const std::string& name, const float* value);

    void SetFloat(const std::string& name, float v);
    void SetFloat(const std::string& name, float x, float y);
    void SetFloat(const std::string& name, float x, float y, float z);
    void SetFloat(const std::string& name, float x, float y, float z, float w);


    void Release();


    void print();

    bool findUniform(const std::string& name) const;

    int getUniform(const std::string& name);
    int getAttribute(const std::string& name);


    int getUniformLocation(const std::string& uniformName) const;
    int getAttribLocation(const std::string& attribName) const;

    bool addUniform(const char* name);
    bool addAttribute(const char* name);

    bool ContainsUniform(const std::string& name) const;

    void LoadDefaults();

private:
    u32 m_program;
    int m_numAttributes;
    int m_numUniforms;
    bool m_panic;


private:
    void checkCompileErrors(unsigned int shader, const std::string& type);

    Shader& operator=(const Shader& other) = delete;
    Shader(const Shader& other) = delete;

    std::map<std::string, int> m_uniforms;
    std::map<std::string, int> m_attributes;
};


class ShaderManager {
public:
 
    u32 GetTotalShaders() { return m_loadedShaders.size(); }


    bool Create(const char* vShaderCode, const char* fShaderCode, const std::string& name);

    bool Load(const char* vShaderCode, const char* fShaderCode, const std::string& name);

    Shader* Get(const std::string& name);
    Shader* Get(u32 index);

    bool Exists(const std::string &name);

    bool Add(Shader* texture, const std::string& name);

    bool Remove(const std::string& name);

    void Clear();


    static ShaderManager& Instance();
    static ShaderManager* InstancePtr();

private:
    std::map<std::string, Shader*> m_shaders;
    std::vector<Shader*> m_loadedShaders;


    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;


    ShaderManager();
    ~ShaderManager();
};