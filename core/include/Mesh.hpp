
#pragma once


#include "Config.hpp"
#include "Texture.hpp"
#include "Device.hpp"
#include "glad/glad.h"


class SceneNode;
class Scene;
class Model;

const int MAX_TEXTURE_COUNT = 4;
const int VBO_POSITION = 0x00000001;
const int VBO_NORMAL = 0x00000002;
const int VBO_COLOR = 0x00000004;
const int VBO_TANGENT = 0x00000008;
const int VBO_BITANGENT = 0x00000010;
const int VBO_BLENDWEIGHTS = 0x00000020;
const int VBO_BLENDINDICES = 0x00000040;
const int VBO_TEXCOORD0 = 0x00000080;
const int VBO_TEXCOORD1 = 0x00000100;
const int VBO_INDICES = 0x00000200;



class CORE_PUBLIC Material 
{

    bool cullFace;
    Texture* textures[MAX_TEXTURE_COUNT];

public:
    Material();

    Material* SetTexture(u32 index, Texture* texture);

private:
    void Bind();
    friend class Model;
    friend class Scene;

};


class CORE_PUBLIC VertexFormat {
public:
    enum Usage
    {
        POSITION = 1,
        NORMAL = 2,
        COLOR = 3,
        TANGENT = 4,
        BITANGENT = 5,
        BLENDWEIGHTS = 6,
        BLENDINDICES = 7,
        TEXCOORD0 = 8,
        TEXCOORD1 = 9,
        TEXCOORD2 = 10,
        TEXCOORD3 = 11,
        TEXCOORD4 = 12,
        TEXCOORD5 = 13,
        TEXCOORD6 = 14,
        TEXCOORD7 = 15
    };


    class Element {
    public:
        Usage usage;
        unsigned int size;
        Element();
        Element(Usage usage, unsigned int size);
        bool operator==(const Element& e) const;
        bool operator!=(const Element& e) const;
    };


    VertexFormat() { _vertexSize = 0; };


    VertexFormat(const Element* elements, unsigned int elementCount);
    ~VertexFormat();

    const Element& getElement(unsigned int index) const;
    unsigned int getElementCount() const;
    unsigned int getVertexSize() const;
    bool operator==(const VertexFormat& f) const;
    bool operator!=(const VertexFormat& f) const;

private:
    std::vector<Element> _elements;
    unsigned int _vertexSize;
};


enum PrimitiveType
{
    TRIANGLES = GL_TRIANGLES,
    TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
    LINES = GL_LINES,
    LINE_STRIP = GL_LINE_STRIP,
    POINTS = GL_POINTS
};


class CORE_PUBLIC Mesh {
public:
    Mesh(const VertexFormat& vertexFormat, u32 material = 0,
         bool dynamic = false);
    virtual ~Mesh();

    int GetMaterial() const { return m_material; }
    void SetMaterial(u32 material) { m_material = material; }

    void Render(u32 mode, u32 count);
    void Render(u32 mode);
    void Render();


    int AddVertex(const Vec3& position);
    int AddVertex(float x, float y, float z);

    const void* GetVertices() const;
    void* GetVertices();

    void* GetIndices();
    const void* GetIndices() const;

    bool CastsShadows() const { return m_castsShadows; }
    void SetCastsShadows(bool castsShadows) { m_castsShadows = castsShadows; }


    void TexturePlanarMapping(float resolution = 0.001f);
    void TexturePlanarMapping(float resolutionS, float resolutionT, u8 axis,
                              const Vec3& offset);
    // Changes backfacing triangles to frontfacing
    void FlipFaces();
    void FlipNormals();


    int AddVertex(const Vec3& position, const Vec2& texCoord);
    int AddVertex(float x, float y, float z, float u, float v);

    int AddVertex(const Vec3& position, const Vec2& texCoord,
                  const Vec3& normal);

    int AddVertex(const Vec3& position, const Vec2& texCoord,
                  const Vec3& normal, const Color& color);

                  
    int AddVertex(float x, float y, float z, float u, float v, float nx,
                  float ny, float nz);
    int AddVertex(const Vec3 &position, const Color &color);


    void VertexPosition(u32 index, const Vec3& position);
    void VertexPosition(u32 index, float x, float y, float z);

    void VertexNormal(u32 index, const Vec3& normal);
    void VertexNormal(u32 index, float x, float y, float z);

    void VertexTexCoord(u32 index, const Vec2& texCoord);
    void VertexTexCoord(u32 index, float u, float v);

    int AddFace(u32 v0, u32 v1, u32 v2);

    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    void Transform(const Mat4& transform);
    void TransformTexture(const Mat4& transform);


    void Init();
    void Upload();
    void Release();

    void CalculateNormals();

    void CalculateSmothNormals(bool angleWeighted = false);

    void CalculateTangents();

    void CalculateBoundingBox();

    BoundingBox GetBoundingBox() const { return m_boundingBox; }


    bool CheckIntersection(const Mat4 &mat, const Vec3 &rayOrig, const Vec3 &rayDir, Vec3 &intsPos ) const;

    void Clear();

    u32 GetVertexCount() const { return (u32)positions.size(); }
    u32 GetIndexCount() const { return (u32)indices.size(); }


private:
    struct VertexBuffer
    {
        std::string name;
        s32 size;
        s32 usage;
        unsigned int id;
        VertexBuffer()
        {
            size = 0;
            usage = 0;
            id = 0;
            name = "";
        }
    };
    void AddBuffer(VertexBuffer* buffer) { this->buffers.push_back(buffer); }

private:
    friend class Scene;
    u32 m_material;
    u32 m_stride;
    bool m_dynamic;
    u32 flags;
    bool isDirty;
    std::string m_name;
    BoundingBox m_boundingBox;
    bool m_castsShadows;


    VertexFormat m_vertexFormat;
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> texCoords;
    std::vector<Vec2> texCoords2;
    std::vector<Vec4> tangents;

    std::vector<unsigned char> colors;
    std::vector<unsigned int> indices;
    std::vector<VertexBuffer*> buffers;

    u32 IBO;
    u32 VAO;

    friend class Model;
    friend class Scene;

};


class MeshManager {
public:
    static MeshManager& Instance();
    static MeshManager* InstancePtr();


    Mesh* CreateCube(float size = 1.0f, const std::string& name = "Cube");
    Mesh* CreatePlane(float width, float height, float tileX, float tileY,
                      const std::string& name = "Plane");
    Mesh* CreatePlane(float width, float height, int subdivisionsX,
                      int subdivisionsY, float tileX, float tileY,
                      const std::string& name = "Plane");
    Mesh* CreateSphere(float radius, int segments, int rings,
                       const std::string& name = "Sphere");
    Mesh* CreateCylinder(float radius, float height, int segments,
                         const std::string& name = "Cylinder");
    Mesh* CreateCapsule(float radius, float height, int segments, int rings,
                        const std::string& name = "Capsule");
    Mesh* CreateTorus(float majorRadius, float minorRadius,
                                   int majorSegments, int minorSegments,
                                   const std::string& name = "Torus");
    Mesh* CreateCone(float radius, float height, int segments,
                                  const std::string& name = "Cone");
    Mesh* CreateArrow(float length, float headSize,
                                   const std::string& name = "Arrow");
    Mesh* CreateGizmoAxis(float length, float headSize,
                                       const Vec3& direction, 
                                       const std::string& name = "GizmoAxis");


    bool Add(Mesh* mesh, const std::string& name);
    void Clear();
    bool Exists(const std::string& name);
    Mesh* Get(const std::string& name);
    Mesh* Get(u32 index);

private:
    MeshManager() {};
    ~MeshManager() {};
    MeshManager(const MeshManager&) = delete;
    MeshManager& operator=(const MeshManager&) = delete;
    MeshManager(MeshManager&&) = delete;
    MeshManager& operator=(MeshManager&&) = delete;

    std::vector<Mesh*> m_meshes;
    std::unordered_map<std::string, Mesh*> m_meshesByName;
};
