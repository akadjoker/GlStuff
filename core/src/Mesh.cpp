
#include "pch.h"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "glad/glad.h"

Material::Material() 
{
    for (int i = 0; i < MAX_TEXTURE_COUNT; i++)
    {
        textures[i] = nullptr;
    }
    cullFace = true;
}

Material *Material::SetTexture(u32 index, Texture *texture) 
{
    textures[index] = texture;
    return this;
}


void Material::Bind()
{   
   
    for (int i = 0; i < MAX_TEXTURE_COUNT; i++)
    {
        if (textures[i] == nullptr) continue;
        textures[i]->Use(i);
    }
}


//*******************************************************
//
//*******************************************************



VertexFormat::VertexFormat(const Element* elements, unsigned int elementCount)
    : _vertexSize(0)
{
    for (unsigned int i = 0; i < elementCount; ++i)
    {
        Element element;
        memcpy(&element, &elements[i], sizeof(Element));
        _elements.push_back(element);
        _vertexSize += element.size * sizeof(float);
    }
}

VertexFormat::~VertexFormat(){}

const VertexFormat::Element& VertexFormat::getElement(unsigned int index) const{return _elements[index];}

unsigned int VertexFormat::getElementCount() const{    return (unsigned int)_elements.size();}
unsigned int VertexFormat::getVertexSize() const{    return _vertexSize;}

bool VertexFormat::operator == (const VertexFormat& f) const
{
    if (_elements.size() != f._elements.size())
        return false;
    for (size_t i = 0, count = _elements.size(); i < count; ++i)
    {
        if (_elements[i] != f._elements[i])
            return false;
    }

    return true;
}
bool VertexFormat::operator != (const VertexFormat& f) const{    return !(*this == f);}
VertexFormat::Element::Element() :    usage(POSITION), size(0){}
VertexFormat::Element::Element(Usage usage, unsigned int size) :    usage(usage), size(size){}
bool VertexFormat::Element::operator == (const VertexFormat::Element& e) const{    return (size == e.size && usage == e.usage);}
bool VertexFormat::Element::operator != (const VertexFormat::Element& e) const{    return !(*this == e);}

//*******************************************************
//
//*******************************************************



Mesh::Mesh(const VertexFormat &vertexFormat, u32 material, bool dynamic)
{
    m_vertexFormat = vertexFormat;
    m_material = material;
    m_dynamic = dynamic;
    flags = VBO_POSITION | VBO_INDICES;
    m_stride = vertexFormat.getVertexSize();
    isDirty = true;
    IBO=0;
    VAO=0;
    m_name = "Mesh";
    m_castsShadows = true;
    Init();
   
    
}

Mesh::~Mesh()
{
    Release();
}

void Mesh::Transform(const Mat4 &transform)
{

    bool hasNormals = normals.size() == positions.size();
    for (u32 i = 0; i < positions.size(); ++i) 
    {
        positions[i] = Mat4::Transform(transform,positions[i]);
     
        if (hasNormals) 
        {
            normals[i] = Mat4::TransformNormal(transform,normals[i]);
        }
        
    }
    flags |= VBO_POSITION;
    if (hasNormals) 
    {
        flags |= VBO_NORMAL;
    }
    CalculateBoundingBox();

    isDirty = true;
}

void Mesh::TransformTexture(const Mat4 &transform)
{

    for (u32 i = 0; i < texCoords.size(); ++i) 
    {
        Vec3 tex;
        tex.set(texCoords[i].x,texCoords[i].y,0.0f);

        tex = Mat4::Transform(transform,tex);

        texCoords[i].set(tex.x,tex.y);
    }
    flags |= VBO_TEXCOORD0;
    isDirty = true;
}

void Mesh::Init()
{

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    isDirty = true;


     for (u32 j = 0; j <m_vertexFormat.getElementCount(); ++j)
    {
            const VertexFormat::Element& e = m_vertexFormat.getElement(j);           
            if (e.usage == VertexFormat::POSITION) 
            {
                flags |= VBO_POSITION;
                VertexBuffer * buffer = new VertexBuffer();
                glGenBuffers(1, &buffer->id);
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glEnableVertexAttribArray(j);
                glVertexAttribPointer(j, 3, GL_FLOAT, GL_FALSE, (GLint)sizeof(Vec3), 0);

                buffer->size  = e.size;
                buffer->usage = e.usage;
                buffer->name = "POSITION";
            //    Log(1,"POSITION");
                
                AddBuffer(buffer);
            }
            else
            if (e.usage == VertexFormat::TEXCOORD0) 
            {
                flags |= VBO_TEXCOORD0;

                VertexBuffer * buffer = new VertexBuffer();
                glGenBuffers(1, &buffer->id);
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glEnableVertexAttribArray(j);
                glVertexAttribPointer(j, 2, GL_FLOAT, GL_FALSE, (GLint)sizeof(Vec2), 0);
                buffer->size  = e.size;
                buffer->usage = e.usage;
                buffer->name = "TEXCOORD0";
              //  Log(1,"TEXCOORD0");
                AddBuffer(buffer);

                

            }
            else if (e.usage == VertexFormat::TEXCOORD1) 
            {
                flags |= VBO_TEXCOORD1;

                VertexBuffer * buffer = new VertexBuffer();
                glGenBuffers(1, &buffer->id);
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glEnableVertexAttribArray(j);
                glVertexAttribPointer(j, 2, GL_FLOAT, GL_FALSE, (GLint)sizeof(Vec2), 0);
                buffer->size  = e.size;
                buffer->usage = e.usage;
                buffer->name = "TEXCOORD1";
              //  Log(1,"TEXCOORD0");
                AddBuffer(buffer);

                

            }
            else if (e.usage == VertexFormat::NORMAL) 
            {
                flags |= VBO_NORMAL;
                VertexBuffer * buffer = new VertexBuffer();
                glGenBuffers(1, &buffer->id);
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glEnableVertexAttribArray(j);
                glVertexAttribPointer(j, 3, GL_FLOAT, GL_FALSE, (GLint)sizeof(Vec3), 0);
                buffer->size  = e.size;
                buffer->usage = e.usage;
                buffer->name = "NORMAL";
            //   Log(1,"NORMAL");
                AddBuffer(buffer);
            } else
                
            if (e.usage == VertexFormat::TANGENT) 
            {
                flags |= VBO_TANGENT;
               
                VertexBuffer * buffer = new VertexBuffer();
                glGenBuffers(1, &buffer->id);
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
    
                glEnableVertexAttribArray(j);
                glVertexAttribPointer(j, 4, GL_FLOAT, GL_FALSE, (GLint)sizeof(Vec4), 0);

                buffer->size  = e.size;
                buffer->usage = e.usage;
                buffer->name = "TANGENT";
                
                AddBuffer(buffer);
            } else          
            if (e.usage == VertexFormat::COLOR)
            {
                flags |= VBO_COLOR;
                VertexBuffer * buffer = new VertexBuffer();
                glGenBuffers(1, &buffer->id);
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
           
                glEnableVertexAttribArray(j);
               // glVertexAttribPointer(j, (GLint)e.size, GL_FLOAT, GL_FALSE, (GLint)e.size * sizeof(float), (void*)(0));
                glVertexAttribPointer(j, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
                buffer->size  = e.size;
                buffer->usage = e.usage;
                buffer->name = "COLOR";

           
                AddBuffer(buffer);
            } 
        }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::Upload()
{
    // Fazer bind do VAO para configurar o estado
    glBindVertexArray(VAO);
    
    for (u32 i = 0; i < buffers.size(); ++i)
    {
        VertexBuffer *buffer = buffers[i];
        
        if (buffer->usage == VertexFormat::POSITION)
        {
            if (flags & VBO_POSITION)
            {
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(Vec3), positions.data(), m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            }
        }
        else if (buffer->usage == VertexFormat::TEXCOORD0)
        {
            if (flags & VBO_TEXCOORD0)
            {
                if (texCoords.size() != positions.size())
                {
                    texCoords.resize(positions.size(), Vec2(0.0f, 0.0f));
                }
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(Vec2), texCoords.data(), m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            }
        }
        else if (buffer->usage == VertexFormat::TEXCOORD1)
        {
            if (flags & VBO_TEXCOORD1)
            {
                if (texCoords2.size() != positions.size())
                {
                    texCoords2.resize(positions.size(), Vec2(0.0f, 0.0f));
                }
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glBufferData(GL_ARRAY_BUFFER, texCoords2.size() * sizeof(Vec2), texCoords2.data(), m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            }
        }
        else if (buffer->usage == VertexFormat::NORMAL)
        {
            if (flags & VBO_NORMAL)
            {
                if (normals.size() != positions.size())
                {
                    normals.resize(positions.size(), Vec3(0.0f, 0.0f, 1.0f)); // Normal padrão para cima
                }
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(Vec3), normals.data(), m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            }
        }
        else if (buffer->usage == VertexFormat::COLOR)
        {
            if (flags & VBO_COLOR) // Adicionar verificação de flag
            {
                if ((colors.size() / 4) != positions.size())
                {
                    const size_t vcount = positions.size();
                    colors.resize(vcount * 4, 255); // RGBA = 255
                }
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(unsigned char), colors.data(), m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            }
        }
        else if (buffer->usage == VertexFormat::TANGENT)
        {
            if (flags & VBO_TANGENT) // Verificação ANTES do upload
            {
                if (tangents.size() != positions.size())
                {
                    tangents.resize(positions.size(), Vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Tangente padrão
                }
                glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
                glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(Vec4), tangents.data(), m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            }
        }
    }
    
    // Upload de índices
    if (flags & VBO_INDICES)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }
    
    // Limpar bindings
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    // Reset flags após upload bem-sucedido
    flags = 0;
    isDirty = false;
}

void Mesh::FlipFaces()
{
    const u32 idxcnt = (u32)indices.size();
    if (idxcnt == 0) return;
    
     unsigned int *idx = indices.data();

    for (u32 i = 0; i < idxcnt; i += 3) 
    {
       const unsigned int tmp = idx[i+1];
        idx[i+1] = idx[i + 2];
        idx[i + 2] = tmp;
    }
    isDirty = true;
    flags |= VBO_INDICES;

}

void Mesh::TexturePlanarMapping(float resolution)
{

     const u32 idxcnt = (u32)indices.size();
     const u32 texcnt = (u32)texCoords.size();


    if (idxcnt == 0) return;
    if (texcnt < positions.size()) 
    {
        for (size_t i = 0; i < positions.size(); ++i) 
        {
            texCoords.push_back(Vec2(0.0f, 0.0f));
        }
    }
    


    for (u32 i = 0; i < idxcnt; i += 3) 
    {
        Plane3D plane(positions[indices[i]], positions[indices[i + 1]], positions[indices[i + 2]]);
        Vec3 normal = plane.normal;
        normal.x = fabs(normal.x);
        normal.y = fabs(normal.y);
        normal.z = fabs(normal.z);

        if (normal.x > normal.y && normal.x > normal.z) 
        {
            for (u32 j = 0; j < 3; ++j) 
            {
                texCoords[indices[i + j]].x = positions[indices[i + j]].y * resolution;
                texCoords[indices[i + j]].y = positions[indices[i + j]].z * resolution;
            }
        } else 
        if (normal.y > normal.x && normal.y > normal.z) 
        {
            for (u32 j = 0; j < 3; ++j) 
            {
                texCoords[indices[i + j]].x = positions[indices[i + j]].x * resolution;
                texCoords[indices[i + j]].y = positions[indices[i + j]].z * resolution;
            }
        } else 
        {
            for (u32 j = 0; j < 3; ++j) 
            {
                texCoords[indices[i + j]].x = positions[indices[i + j]].x * resolution;
                texCoords[indices[i + j]].y = positions[indices[i + j]].y * resolution;
            }
        }
           
    }



    flags |= VBO_TEXCOORD0;
    isDirty = true;
}
void Mesh::TexturePlanarMapping(float resolutionS, float resolutionT, u8 axis, const Vec3& offset)
{
     const u32 idxcnt = (u32)indices.size();
     const u32 texcnt = (u32)texCoords.size();
     if (idxcnt == 0 || texcnt==0) return;


    for (u32 i = 0; i < idxcnt; i += 3)
    {

        if (axis==0)
        {
                for (u32 j = 0; j < 3; ++j) 
                {
                    texCoords[indices[i + j]].x =0.5f +(positions[indices[i + j]].z + offset.z) * resolutionS;
                    texCoords[indices[i + j]].y =0.5f -(positions[indices[i + j]].y + offset.y) * resolutionT;
                

                }
        } else if (axis==1)
        {
                for (u32 j = 0; j < 3; ++j) 
                {
                    texCoords[indices[i + j]].x =0.5f +(positions[indices[i + j]].x + offset.x) * resolutionS;
                    texCoords[indices[i + j]].y =1.0f -(positions[indices[i + j]].z + offset.z) * resolutionT;
                }
        } else if (axis==2)
        {
                for (u32 j = 0; j < 3; ++j) 
                {
                    texCoords[indices[i + j]].x =0.5f +(positions[indices[i + j]].x + offset.x) * resolutionS;
                    texCoords[indices[i + j]].y =0.5f -(positions[indices[i + j]].y + offset.y) * resolutionT;
                }
        }
    
    }



    flags |= VBO_TEXCOORD0;
    isDirty = true;
}



void Mesh::Release()
{

    if (VAO != 0) 
    {
        glDeleteVertexArrays(1, &VAO);
    }
    if (IBO != 0) 
    {
        glDeleteBuffers(1, &IBO);
    }
    for (u32 i = 0; i < buffers.size(); ++i) 
    {
        VertexBuffer *buffer = buffers[i];
    
        glDeleteBuffers(1, &buffer->id);
        buffer->id = 0;
        delete buffer;
    }

    buffers.clear();

}

void Mesh::Render(u32 mode,u32 count)
{
    if (isDirty) 
    {
        Upload();
    }

  

    glBindVertexArray(VAO);
    glDrawElements(mode, count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::Render(u32 mode)
{
   
    this->Render(mode, this->GetIndexCount());
}
void Mesh::Render()
{
    this->Render(GL_TRIANGLES, this->GetIndexCount());
}

const void* Mesh::GetVertices() const
{
	return positions.data();
}
void* Mesh::GetVertices()
{
	return positions.data();
}

void* Mesh::GetIndices()
{
    	return indices.data();

}
const void* Mesh::GetIndices() const
{
		return indices.data();
}


int Mesh::AddVertex(const Vec3 &position)
{
    positions.push_back(position);
    flags |= VBO_POSITION;
    texCoords.push_back(Vec2(1.0f));
    normals.push_back(Vec3(1.0f));

    m_boundingBox.AddPoint(position);
    isDirty = true;
    return (int)positions.size() - 1;
}

int Mesh::AddVertex(float x, float y, float z)
{
    Vec3 position(x, y, z);
    positions.push_back(position);
    texCoords.push_back(Vec2(1.0f));
    normals.push_back(Vec3(1.0f));
    flags |= VBO_POSITION;
    isDirty = true;
    m_boundingBox.AddPoint(position);
    return (int)positions.size() - 1;
}

int Mesh::AddVertex(const Vec3 &position,const Vec2 &texCoord)
{
    positions.push_back(position);
    texCoords.push_back(texCoord);
    m_boundingBox.AddPoint(position);
    flags |= VBO_POSITION | VBO_TEXCOORD0;
     isDirty = true;
    return (int)positions.size() - 1;
}

int Mesh::AddVertex(float x, float y, float z, float u, float v)
{
    Vec3 position(x, y, z);
    positions.push_back(position);
    m_boundingBox.AddPoint(position);
    texCoords.push_back(Vec2(u, v));
    flags |= VBO_POSITION | VBO_TEXCOORD0;
     isDirty = true;
    return (int)positions.size() - 1;
}

int Mesh::AddVertex(const Vec3 &position, const Color &color)
{

    positions.push_back(position);
    colors.push_back(color.r);
    colors.push_back(color.g);
    colors.push_back(color.b);
    colors.push_back(color.a);
    m_boundingBox.AddPoint(position);
    flags |= VBO_POSITION | VBO_COLOR;
     isDirty = true;
    return (int)positions.size() - 1;
}
int Mesh::AddVertex(const Vec3 &position,const Vec2 &texCoord,const Vec3 &normal)
{

    positions.push_back(position);
    m_boundingBox.AddPoint(position);
    texCoords.push_back(texCoord);
    normals.push_back(normal);
    flags |= VBO_POSITION | VBO_TEXCOORD0 | VBO_NORMAL;
     isDirty = true;
    return (int)positions.size() - 1;
}

int Mesh::AddVertex(const Vec3 &position, const Vec2 &texCoord, const Vec3 &normal, const Color &color)
{

    positions.push_back(position);
    m_boundingBox.AddPoint(position);
    texCoords.push_back(texCoord);
    normals.push_back(normal);
    colors.push_back(color.r);
    colors.push_back(color.g);
    colors.push_back(color.b);
    colors.push_back(color.a);
    flags |= VBO_POSITION | VBO_TEXCOORD0 | VBO_NORMAL | VBO_COLOR;
     isDirty = true;
    return (int)positions.size() - 1;
}

int Mesh::AddVertex(float x, float y, float z, float u, float v, float nx, float ny, float nz)
{
     Vec3 position(x, y, z);
    positions.push_back(position);
    m_boundingBox.AddPoint(position);

    texCoords.push_back(Vec2(u, v));
    normals.push_back(Vec3(nx, ny, nz));
    flags |= VBO_POSITION | VBO_TEXCOORD0 | VBO_NORMAL;
     isDirty = true;
    return (int)positions.size() - 1;
}



void Mesh::VertexPosition(u32 index, const Vec3 &position)
{
    m_boundingBox.AddPoint(position);
    positions[index] = position;
    flags |= VBO_POSITION;
     isDirty = true;
}

void Mesh::VertexPosition(u32 index, float x, float y, float z)
{
    positions[index] = Vec3(x, y, z);
     m_boundingBox.AddPoint(positions[index]);
    flags |= VBO_POSITION;
     isDirty = true;
}

void Mesh::VertexNormal(u32 index, const Vec3 &normal)
{
    normals[index] = normal;
    flags |= VBO_NORMAL;
     isDirty = true;
}

void Mesh::VertexNormal(u32 index, float x, float y, float z)
{
    normals[index] = Vec3(x, y, z);
    flags |= VBO_NORMAL;
     isDirty = true;
}

void Mesh::VertexTexCoord(u32 index, const Vec2 &texCoord)
{
    texCoords[index] = texCoord;
    flags |= VBO_TEXCOORD0;
     isDirty = true;
}

void Mesh::VertexTexCoord(u32 index, float u, float v)
{
    texCoords[index] = Vec2(u, v);
    flags |= VBO_TEXCOORD0;
     isDirty = true;
}

int Mesh::AddFace(u32 v0, u32 v1, u32 v2)
{
    indices.push_back(v0);
    indices.push_back(v1);
    indices.push_back(v2);
    flags |= VBO_INDICES;
     isDirty = true;
    return (int)indices.size() - 3;
}



void Mesh::CalculateNormals()
{

    if (indices.size() > 0) 
    {

        if (normals.size() != positions.size()) 
        {
            for (size_t i = 0; i < positions.size(); ++i) 
            {
                normals.push_back(Vec3(0.0f, 0.0f, 0.0f));
            }
        }
        for (u32 i = 0; i < GetIndexCount(); i += 3)
        {
            Plane3D plane = Plane3D(positions[indices[i]], positions[indices[i + 1]], positions[indices[i + 2]]);
          
            Vec3 normal = plane.normal;
            normals[indices[i]] = normal;
            normals[indices[i + 1]] = normal;
            normals[indices[i + 2]] = normal;

        }


         flags |= VBO_NORMAL;
         isDirty = true;
    }

       
}

bool Mesh::CheckIntersection(const Mat4 &mat, const Vec3 &rayOrig,
                             const Vec3 &rayDir, Vec3 &intsPos) const
{
    return false;
}

void Mesh::Clear()
{
    positions.clear();
    normals.clear();
    texCoords.clear();
    indices.clear();
    colors.clear();
    m_boundingBox.Clear();
}


 

void Mesh::CalculateSmothNormals(bool angleWeighted)
{
    if (indices.empty()) return;

 
    normals.assign(positions.size(), Vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        const u32 i0 = indices[i+0], i1 = indices[i+1], i2 = indices[i+2];
        const Vec3 &v0 = positions[i0], &v1 = positions[i1], &v2 = positions[i2];

        // normal de face (usa NÃO normalizada p/ ponderar por área; normaliza no fim)
        Vec3 n = Vec3::Cross(v1 - v0, v2 - v0);
        const float len2 = n.x*n.x + n.y*n.y + n.z*n.z;
        if (len2 < 1e-20f) continue; // triângulo degenerado

        Vec3 w = angleWeighted ? Vec3::GetAngleWeights(v0, v1, v2) : Vec3(1.f);

        normals[i0] += n * w.x;
        normals[i1] += n * w.y;
        normals[i2] += n * w.z;
    }

    for (auto &nn : normals) nn = Vec3::Normalize(nn);

    flags |= VBO_NORMAL;
    isDirty = true;
}

void Mesh::CalculateTangents()
{
    if (indices.empty()) return;

 
    if (texCoords.size() != positions.size()) return;            // sem UVs não dá
    if (normals.size()   != positions.size()) CalculateSmothNormals(true);

    tangents.assign(positions.size(), Vec4(0.0f));               // T.xyz acumulado, .w = sinal
    std::vector<Vec3> Bacc(positions.size(), Vec3(0.0f));        // acumula B só para calcular o sinal

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        const u32 i0 = indices[i+0], i1 = indices[i+1], i2 = indices[i+2];
        const Vec3 &v0 = positions[i0], &v1 = positions[i1], &v2 = positions[i2];
        const Vec2 &uv0 = texCoords[i0], &uv1 = texCoords[i1], &uv2 = texCoords[i2];

        const Vec3 e1 = v1 - v0;
        const Vec3 e2 = v2 - v0;
        const Vec2 duv1 = uv1 - uv0;
        const Vec2 duv2 = uv2 - uv0;

        const float denom = duv1.x * duv2.y - duv2.x * duv1.y;
        if (fabsf(denom) < 1e-20f) continue;                     // UV degenerado: ignora
        const float f = 1.0f / denom;

        const Vec3 T = (e1 * duv2.y - e2 * duv1.y) * f;
        const Vec3 B = (e2 * duv1.x - e1 * duv2.x) * f;

        // acumula T em cada vértice
        tangents[i0].x += T.x; tangents[i0].y += T.y; tangents[i0].z += T.z;
        tangents[i1].x += T.x; tangents[i1].y += T.y; tangents[i1].z += T.z;
        tangents[i2].x += T.x; tangents[i2].y += T.y; tangents[i2].z += T.z;

        // acumula B para calcular o sinal depois
        Bacc[i0] += B; Bacc[i1] += B; Bacc[i2] += B;
    }

    // ortonormaliza e calcula handedness
    for (size_t i = 0; i < positions.size(); ++i)
    {
        const Vec3 N = normals[i];
        Vec3 T = Vec3(tangents[i].x, tangents[i].y, tangents[i].z);

        // Gram–Schmidt: T ⟂ N
        T = (T - N * Vec3::Dot(N, T)).normalized();

        float h = 1.0f;
        if (T.length_squared() > 0.0f && Bacc[i].length_squared() > 0.0f) {
            // sinal: se (N × T) aponta para o mesmo lado de B, h=+1; senão h=-1
            h = (Vec3::Dot(Vec3::Cross(N, T), Bacc[i]) < 0.0f) ? -1.0f : 1.0f;
        }

        tangents[i] = Vec4(T.x, T.y, T.z, h);
    }

    flags |= VBO_TANGENT;  // marca para fazer upload
    isDirty = true;
}

void Mesh::CalculateBoundingBox()
{
    m_boundingBox.Clear();
    for (size_t i = 0; i < positions.size(); ++i) 
    {
        m_boundingBox.AddPoint(positions[i]);
    }
}

MeshManager &MeshManager::Instance()
{
    static MeshManager instance;
    return instance;
}

MeshManager *MeshManager::InstancePtr() 
{
    return &Instance();
}


bool MeshManager::Add(Mesh *mesh, const std::string &name)
{
    auto it = m_meshesByName.find(name);
    if (it != m_meshesByName.end()) 
    {
        return false;
    }
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    return true;
}

void MeshManager::Clear()
{
    for (auto mesh : m_meshes) 
    {
        mesh->Release();
        delete mesh;
    }
    m_meshes.clear();
    m_meshesByName.clear();
}

bool MeshManager::Exists(const std::string &name) 
{
    auto it = m_meshesByName.find(name);
    if (it != m_meshesByName.end()) 
    {
        return true;
    }
    return false;
}

Mesh *MeshManager::Get(const std::string &name) 
{
    auto it = m_meshesByName.find(name);
    if (it != m_meshesByName.end()) 
    {
        return it->second;
    }
    return nullptr;
}

Mesh *MeshManager::Get(u32 index) 
{
    if (index < m_meshes.size()) 
    {
        return m_meshes[index];
    }
    return nullptr;
}

Mesh *MeshManager::CreateCube(float size, const std::string &name)
{
    VertexFormat::Element VertexElements[] = {
        VertexFormat::Element(VertexFormat::POSITION, 3),
        VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
        VertexFormat::Element(VertexFormat::NORMAL, 3),
        VertexFormat::Element(VertexFormat::TANGENT, 4),
    };
    
    Mesh *mesh = new Mesh(VertexFormat(VertexElements, 4), 0, false);
    
    float w = size * 0.5f, h = size * 0.5f, d = size * 0.5f;
    

    struct CubeFace 
    {
        Vec3 positions[4];
        Vec2 uvs[4];
        Vec3 normal;
    };
    
    CubeFace faces[6] = {
        // Front Face (Z+)
        {{{-w, -h,  d}, { w, -h,  d}, { w,  h,  d}, {-w,  h,  d}},
         {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
         {0, 0, 1}},
        
        // Back Face (Z-)
        {{{ w, -h, -d}, {-w, -h, -d}, {-w,  h, -d}, { w,  h, -d}},
         {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
         {0, 0, -1}},
        
        // Right Face (X+)
        {{{ w, -h,  d}, { w, -h, -d}, { w,  h, -d}, { w,  h,  d}},
         {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
         {1, 0, 0}},
        
        // Left Face (X-)
        {{{-w, -h, -d}, {-w, -h,  d}, {-w,  h,  d}, {-w,  h, -d}},
         {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
         {-1, 0, 0}},
        
        // Top Face (Y+)
        {{{-w,  h,  d}, { w,  h,  d}, { w,  h, -d}, {-w,  h, -d}},
         {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
         {0, 1, 0}},
        
        // Bottom Face (Y-)
        {{{-w, -h, -d}, { w, -h, -d}, { w, -h,  d}, {-w, -h,  d}},
         {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
         {0, -1, 0}}
    };
    
  
    for (int faceIndex = 0; faceIndex < 6; ++faceIndex) 
    {
        const CubeFace& face = faces[faceIndex];
        const int base = mesh->GetVertexCount();
        
        // 4 vértices da face
        for (int i = 0; i < 4; ++i) 
        {
            mesh->AddVertex(face.positions[i], face.uvs[i], face.normal);
        }
      
        mesh->AddFace(base + 0, base + 1, base + 2);
        mesh->AddFace(base + 0, base + 2, base + 3);
    }
    
    mesh->CalculateSmothNormals();
    mesh->CalculateTangents();
    mesh->Upload();
    
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    
    return mesh;
}

Mesh* MeshManager::CreatePlane(float width, float depth, int subdivisionsX, int subdivisionsY,
                               float tileX, float tileY, const std::string& name)
{
    VertexFormat::Element elems[] = {
        { VertexFormat::POSITION, 3 },
        { VertexFormat::TEXCOORD0, 2 },
        { VertexFormat::NORMAL,   3 },
        { VertexFormat::TANGENT,  4 },
    };
    Mesh* mesh = new Mesh(VertexFormat(elems, 4), 0, false);

    subdivisionsX = std::max(1, subdivisionsX);
    subdivisionsY = std::max(1, subdivisionsY);

    const int vertsX = subdivisionsX + 1;
    const int vertsY = subdivisionsY + 1;

    const float stepX   = width  / (float)subdivisionsX;
    const float stepZ   = depth  / (float)subdivisionsY;
    const float uvStepX = tileX  / (float)subdivisionsX;
    const float uvStepY = tileY  / (float)subdivisionsY;

    const float startX = -width * 0.5f;
    const float startZ = -depth * 0.5f;
    const Vec3  normal(0, 1, 0);

 

    // vértices
    for (int y = 0; y < vertsY; ++y) {
        for (int x = 0; x < vertsX; ++x) {
            Vec3 p(startX + x * stepX, 0.0f, startZ + y * stepZ);
            Vec2 uv(x * uvStepX, y * uvStepY);          // <-- usa os steps
            mesh->AddVertex(p, uv, normal);
        }
    }

    // índices (CCW)
    for (int y = 0; y < subdivisionsY; ++y) {
        for (int x = 0; x < subdivisionsX; ++x) {
            int i0 =  y      * vertsX + x;
            int i1 =  y      * vertsX + (x+1);
            int i2 = (y + 1) * vertsX + x;
            int i3 = (y + 1) * vertsX + (x+1);
            mesh->AddFace(i0, i2, i1);
            mesh->AddFace(i1, i2, i3);
        }
    }

    mesh->CalculateTangents();
    mesh->Upload();
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    return mesh;
}


 
Mesh *MeshManager::CreatePlane(float width, float height, float tileX, float tileY, const std::string &name)
{
    return CreatePlane(width, height, 1, 1, tileX, tileY, name);
}


Mesh *MeshManager::CreateSphere(float radius, int segments, int rings, const std::string &name)
{
    VertexFormat::Element VertexElements[] = {
        VertexFormat::Element(VertexFormat::POSITION, 3),
        VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
        VertexFormat::Element(VertexFormat::NORMAL, 3),
        VertexFormat::Element(VertexFormat::TANGENT, 4),
    };
    
    Mesh *mesh = new Mesh(VertexFormat(VertexElements, 4), 0, false);
    
    // Garantir valores mínimos
    segments = std::max(3, segments);  // Mínimo 3 segmentos
    rings = std::max(3, rings);        // Mínimo 3 rings
    
    const float PI = 3.14159265359f;
    const float PI2 = PI * 2.0f;
    
    // Gerar vértices
    for (int ring = 0; ring <= rings; ++ring) {
        const float phi = PI * ring / (float)rings;        // Ângulo vertical (0 a π)
        const float y = cosf(phi);                         // Y coordenada
        const float ringRadius = sinf(phi);               // Raio do ring atual
        
        for (int segment = 0; segment <= segments; ++segment) {
            const float theta = PI2 * segment / (float)segments;  // Ângulo horizontal (0 a 2π)
            
            // Posição do vértice
            const float x = ringRadius * cosf(theta);
            const float z = ringRadius * sinf(theta);
            
            Vec3 position = Vec3(x, y, z) * radius;
            
            // Normal (mesmo que a posição normalizada para esfera centrada na origem)
            Vec3 normal = Vec3(x, y, z);  // Já normalizado porque sin²+cos²=1
            
            // Coordenadas UV
            Vec2 uv(
                (float)segment / (float)segments,  // U: 0 a 1
                (float)ring / (float)rings         // V: 0 a 1 
            );
            
            mesh->AddVertex(position, uv, normal);
        }
    }
    
    // Gerar índices
    const int vertsPerRing = segments + 1;
    
    for (int ring = 0; ring < rings; ++ring) {
        for (int segment = 0; segment < segments; ++segment) {
            // Índices dos 4 vértices do quad
            const int current = ring * vertsPerRing + segment;
            const int next = current + vertsPerRing;
            
            const int currentNext = current + 1;
            const int nextNext = next + 1;
            
            // Primeiro triângulo
            mesh->AddFace(current, next, currentNext);
            
            // Segundo triângulo  
            mesh->AddFace(currentNext, next, nextNext);
        }
    }
    
    mesh->CalculateTangents();
    mesh->Upload();
    
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    
    return mesh;
}

Mesh *MeshManager::CreateCylinder(float radius, float height, int segments, const std::string &name)
{
    VertexFormat::Element VertexElements[] = {
        VertexFormat::Element(VertexFormat::POSITION, 3),
        VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
        VertexFormat::Element(VertexFormat::NORMAL, 3),
        VertexFormat::Element(VertexFormat::TANGENT, 4),
    };
    
    Mesh *mesh = new Mesh(VertexFormat(VertexElements, 4), 0, false);
    
    // Garantir mínimo de segmentos
    segments = std::max(3, segments);
    
    const float PI = 3.14159265359f;
    const float PI2 = PI * 2.0f;
    const float halfHeight = height * 0.5f;
    
    // === CORPO DO CILINDRO ===
    
    // Gerar vértices do corpo (2 rings: bottom e top)
    for (int ring = 0; ring <= 1; ++ring) {  // 0 = bottom, 1 = top
        const float y = ring == 0 ? -halfHeight : halfHeight;
        
        for (int segment = 0; segment <= segments; ++segment) 
        {  // <= para fechar a textura
            const float angle = PI2 * segment / (float)segments;
            const float x = cosf(angle) * radius;
            const float z = sinf(angle) * radius;
            
            Vec3 position(x, y, z);
            Vec3 normal(x / radius, 0.0f, z / radius);  // Normal horizontal
            
            // UV: u = ângulo normalizado, v = altura normalizada
            Vec2 uv(
                (float)segment / (float)segments,  // U: 0 a 1
                (float)ring                        // V: 0 (bottom) ou 1 (top)
            );
            
            mesh->AddVertex(position, uv, normal);
        }
    }
    
    // Gerar faces do corpo
    const int vertsPerRing = segments + 1;
    
    for (int segment = 0; segment < segments; ++segment) {
        // Índices dos vértices do quad
        const int bottomLeft = segment;
        const int bottomRight = segment + 1;
        const int topLeft = vertsPerRing + segment;
        const int topRight = vertsPerRing + segment + 1;
        
        // Dois triângulos por quad (CCW quando visto de fora)
        mesh->AddFace(bottomLeft, topLeft, bottomRight);
        mesh->AddFace(bottomRight, topLeft, topRight);
    }
    
    // === TAMPA INFERIOR ===
    
    // Centro da tampa inferior
    const int bottomCenterIndex = mesh->GetVertexCount();
    mesh->AddVertex(Vec3(0, -halfHeight, 0), Vec2(0.5f, 0.5f), Vec3(0, -1, 0));
    
    // Vértices da borda inferior
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = PI2 * segment / (float)segments;
        const float x = cosf(angle) * radius;
        const float z = sinf(angle) * radius;
        
        Vec3 position(x, -halfHeight, z);
        Vec3 normal(0, -1, 0);  // Normal apontando para baixo
        
        // UV circular para a tampa
        Vec2 uv(
            0.5f + (x / radius) * 0.5f,  // Centro + offset baseado em X
            0.5f + (z / radius) * 0.5f   // Centro + offset baseado em Z  
        );
        
        mesh->AddVertex(position, uv, normal);
    }
    
    // Faces da tampa inferior (triângulos do centro para a borda)
    const int bottomRingStart = bottomCenterIndex + 1;
    for (int segment = 0; segment < segments; ++segment) {
        const int current = bottomRingStart + segment;
        const int next = bottomRingStart + segment + 1;
        
        // CCW quando visto de baixo
      //  mesh->AddFace(bottomCenterIndex, next, current);
        mesh->AddFace(current, next, bottomCenterIndex);
    }
    
    // === TAMPA SUPERIOR ===
    
    // Centro da tampa superior
    const int topCenterIndex = mesh->GetVertexCount();
    mesh->AddVertex(Vec3(0, halfHeight, 0), Vec2(0.5f, 0.5f), Vec3(0, 1, 0));
    
    // Vértices da borda superior
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = PI2 * segment / (float)segments;
        const float x = cosf(angle) * radius;
        const float z = sinf(angle) * radius;
        
        Vec3 position(x, halfHeight, z);
        Vec3 normal(0, 1, 0);  // Normal apontando para cima
        
        // UV circular para a tampa
        Vec2 uv(
            0.5f + (x / radius) * 0.5f,
            0.5f + (z / radius) * 0.5f
        );
        
        mesh->AddVertex(position, uv, normal);
    }
    
    // Faces da tampa superior (triângulos do centro para a borda)
    const int topRingStart = topCenterIndex + 1;
    for (int segment = 0; segment < segments; ++segment) {
        const int current = topRingStart + segment;
        const int next = topRingStart + segment + 1;
        
        // CCW quando visto de cima
       // mesh->AddFace(topCenterIndex, current, next);
        mesh->AddFace(next, current, topCenterIndex);
    }
    
    mesh->CalculateTangents();
    mesh->Upload();
    
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    
    return mesh;
}


Mesh *MeshManager::CreateCone(float radius, float height, int segments, const std::string &name)
{
    VertexFormat::Element VertexElements[] = {
        VertexFormat::Element(VertexFormat::POSITION, 3),
        VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
        VertexFormat::Element(VertexFormat::NORMAL, 3),
        VertexFormat::Element(VertexFormat::TANGENT, 4),
    };
    
    Mesh *mesh = new Mesh(VertexFormat(VertexElements, 4), 0, false);
    
    segments = std::max(3, segments);
    
    const float PI = 3.14159265359f;
    const float PI2 = PI * 2.0f;
    const float halfHeight = height * 0.5f;
    
    // Calcular inclinação para normais corretas
    const float slopeLength = sqrtf(radius * radius + height * height);
    const float normalY = radius / slopeLength;      // Componente Y da normal
    const float normalRadius = height / slopeLength; // Componente radial da normal
    
    // === CORPO DO CONE ===
    
    // Ponto superior (apex)
    const int apexIndex = mesh->GetVertexCount();
    mesh->AddVertex(Vec3(0, halfHeight, 0), Vec2(0.5f, 1.0f), Vec3(0, normalY, 0));
    
    // Vértices da base
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = PI2 * segment / (float)segments;
        const float x = cosf(angle) * radius;
        const float z = sinf(angle) * radius;
        
        Vec3 position(x, -halfHeight, z);
        
        // Normal inclinada para o cone
        Vec3 normal(
            (x / radius) * normalRadius,  // X normalizado * componente radial
            normalY,                      // Componente Y
            (z / radius) * normalRadius   // Z normalizado * componente radial
        );
        normal = normal.normalized();
        
        // UV: u baseado no ângulo, v = 0 na base
        Vec2 uv((float)segment / (float)segments, 0.0f);
        
        mesh->AddVertex(position, uv, normal);
    }
    
    // Faces do corpo (triângulos do apex para a base)
    const int baseStart = apexIndex + 1;
    for (int segment = 0; segment < segments; ++segment) {
        const int current = baseStart + segment;
        const int next = baseStart + segment + 1;
        
        // CCW quando visto de fora
        mesh->AddFace(apexIndex, current, next);
    }
    
    // === BASE DO CONE ===
    
    // Centro da base
    const int baseCenterIndex = mesh->GetVertexCount();
    mesh->AddVertex(Vec3(0, -halfHeight, 0), Vec2(0.5f, 0.5f), Vec3(0, -1, 0));
    
    // Vértices da borda da base (com normais apontando para baixo)
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = PI2 * segment / (float)segments;
        const float x = cosf(angle) * radius;
        const float z = sinf(angle) * radius;
        
        Vec3 position(x, -halfHeight, z);
        Vec3 normal(0, -1, 0);  // Normal apontando para baixo
        
        // UV circular para a base
        Vec2 uv(
            0.5f + (x / radius) * 0.5f,
            0.5f + (z / radius) * 0.5f
        );
        
        mesh->AddVertex(position, uv, normal);
    }
    
    // Faces da base (triângulos do centro para a borda)
    const int baseRingStart = baseCenterIndex + 1;
    for (int segment = 0; segment < segments; ++segment) {
        const int current = baseRingStart + segment;
        const int next = baseRingStart + segment + 1;
        
        // CCW quando visto de baixo
        mesh->AddFace(baseCenterIndex, next, current);
    }
    
    mesh->CalculateTangents();
    mesh->Upload();
    
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    
    return mesh;
}


Mesh *MeshManager::CreateTorus(float majorRadius, float minorRadius, int majorSegments, int minorSegments, const std::string &name)
{
    VertexFormat::Element VertexElements[] = {
        VertexFormat::Element(VertexFormat::POSITION, 3),
        VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
        VertexFormat::Element(VertexFormat::NORMAL, 3),
        VertexFormat::Element(VertexFormat::TANGENT, 4),
    };
    
    Mesh *mesh = new Mesh(VertexFormat(VertexElements, 4), 0, false);
    
    // Validação
    majorSegments = std::max(3, majorSegments);
    minorSegments = std::max(3, minorSegments);
    
    const float PI = 3.14159265359f;
    const float PI2 = PI * 2.0f;
    
    // Gerar vértices
    for (int majorSeg = 0; majorSeg <= majorSegments; ++majorSeg) {
        const float majorAngle = PI2 * majorSeg / (float)majorSegments;  // Ângulo ao redor do eixo principal
        const float cosMajor = cosf(majorAngle);
        const float sinMajor = sinf(majorAngle);
        
        // Centro do círculo menor nesta posição
        Vec3 circleCenter(cosMajor * majorRadius, 0.0f, sinMajor * majorRadius);
        
        for (int minorSeg = 0; minorSeg <= minorSegments; ++minorSeg) {
            const float minorAngle = PI2 * minorSeg / (float)minorSegments;  // Ângulo ao redor do círculo menor
            const float cosMinor = cosf(minorAngle);
            const float sinMinor = sinf(minorAngle);
            
            // Posição do vértice
            Vec3 position(
                cosMajor * (majorRadius + minorRadius * cosMinor),  // X
                minorRadius * sinMinor,                             // Y
                sinMajor * (majorRadius + minorRadius * cosMinor)   // Z
            );
            
            // Normal: direção do centro do torus para o vértice
            Vec3 torusCenter(cosMajor * majorRadius, 0.0f, sinMajor * majorRadius);
            Vec3 normal = (position - torusCenter).normalized();
            
            // Coordenadas UV
            Vec2 uv(
                (float)majorSeg / (float)majorSegments,  // U: volta ao redor do torus
                (float)minorSeg / (float)minorSegments   // V: volta ao redor do tubo
            );
            
            mesh->AddVertex(position, uv, normal);
        }
    }
    
    // Gerar faces
    const int vertsPerMajorSeg = minorSegments + 1;
    
    for (int majorSeg = 0; majorSeg < majorSegments; ++majorSeg) {
        for (int minorSeg = 0; minorSeg < minorSegments; ++minorSeg) {
            // Índices dos 4 vértices do quad
            const int current = majorSeg * vertsPerMajorSeg + minorSeg;
            const int next = current + vertsPerMajorSeg;
            const int currentNext = current + 1;
            const int nextNext = next + 1;
            
            // Dois triângulos por quad (CCW quando visto de fora)
            mesh->AddFace(current, next, currentNext);
            mesh->AddFace(currentNext, next, nextNext);
        }
    }
    
    mesh->CalculateTangents();
    mesh->Upload();
    
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    
    return mesh;
}

Mesh *MeshManager::CreateCapsule(float radius, float height, int segments, int rings, const std::string &name)
{
    VertexFormat::Element VertexElements[] = {
        VertexFormat::Element(VertexFormat::POSITION, 3),
        VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
        VertexFormat::Element(VertexFormat::NORMAL, 3),
        VertexFormat::Element(VertexFormat::TANGENT, 4),
    };
    
    Mesh *mesh = new Mesh(VertexFormat(VertexElements, 4), 0, false);
    
    segments = std::max(3, segments);
    rings = std::max(2, rings);  // Mínimo 2 para ter top e bottom hemispheres
    
    const float PI = 3.14159265359f;
    const float PI2 = PI * 2.0f;
    const float cylinderHeight = height - 2.0f * radius; // Altura só da parte cilíndrica
    const float halfCylinderHeight = cylinderHeight * 0.5f;
    
    // === HEMISFÉRIO SUPERIOR ===
    
    for (int ring = 0; ring <= rings / 2; ++ring) {
        const float phi = PI * 0.5f * ring / (float)(rings / 2);  // 0 a π/2
        const float y = cosf(phi) * radius + halfCylinderHeight;
        const float ringRadius = sinf(phi) * radius;
        
        for (int segment = 0; segment <= segments; ++segment) {
            const float theta = PI2 * segment / (float)segments;
            const float x = ringRadius * cosf(theta);
            const float z = ringRadius * sinf(theta);
            
            Vec3 position(x, y, z);
            Vec3 normal = Vec3(x, cosf(phi) * radius, z).normalized();
            
            Vec2 uv(
                (float)segment / (float)segments,
                0.5f + 0.5f * (y - halfCylinderHeight) / radius  // UV da metade para cima
            );
            
            mesh->AddVertex(position, uv, normal);
        }
    }
    
    // === PARTE CILÍNDRICA ===
    
    for (int ring = 0; ring <= 1; ++ring) {  // 2 rings: na junção com as hemisférios
        const float y = ring == 0 ? halfCylinderHeight : -halfCylinderHeight;
        
        for (int segment = 0; segment <= segments; ++segment) {
            const float theta = PI2 * segment / (float)segments;
            const float x = cosf(theta) * radius;
            const float z = sinf(theta) * radius;
            
            Vec3 position(x, y, z);
            Vec3 normal(x / radius, 0.0f, z / radius);  // Normal horizontal
            
            Vec2 uv(
                (float)segment / (float)segments,
                0.5f + 0.5f * (ring == 0 ? -0.1f : 0.1f)  // Slightly above/below center
            );
            
            mesh->AddVertex(position, uv, normal);
        }
    }
    
    // === HEMISFÉRIO INFERIOR ===
    
    for (int ring = 0; ring <= rings / 2; ++ring) {
        const float phi = PI * 0.5f + PI * 0.5f * ring / (float)(rings / 2);  // π/2 a π
        const float y = cosf(phi) * radius - halfCylinderHeight;
        const float ringRadius = sinf(phi) * radius;
        
        for (int segment = 0; segment <= segments; ++segment) {
            const float theta = PI2 * segment / (float)segments;
            const float x = ringRadius * cosf(theta);
            const float z = ringRadius * sinf(theta);
            
            Vec3 position(x, y, z);
            Vec3 normal = Vec3(x, cosf(phi) * radius, z).normalized();
            
            Vec2 uv(
                (float)segment / (float)segments,
                0.5f - 0.5f * (-y - halfCylinderHeight) / radius  // UV da metade para baixo
            );
            
            mesh->AddVertex(position, uv, normal);
        }
    }
    
    // === GERAR FACES ===
    
    const int vertsPerRing = segments + 1;
    const int totalRings = (rings / 2 + 1) + 2 + (rings / 2 + 1); // hemisférios + cilindro + hemisférios
    
    for (int ring = 0; ring < totalRings - 1; ++ring) {
        for (int segment = 0; segment < segments; ++segment) {
            const int current = ring * vertsPerRing + segment;
            const int next = current + vertsPerRing;
            const int currentNext = current + 1;
            const int nextNext = next + 1;
            
            mesh->AddFace(current, next, currentNext);
            mesh->AddFace(currentNext, next, nextNext);
        }
    }
    
    mesh->CalculateTangents();
    mesh->Upload();
    
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    
    return mesh;
}
 Mesh* MeshManager::CreateGizmoAxis(float length, float headSize, const Vec3& direction, const std::string& name)
{
    // Formato minimal: POS + COLOR (estás a desenhar linha/seta sem iluminação)
    VertexFormat::Element elems[] = {
        { VertexFormat::POSITION, 3 },
        { VertexFormat::COLOR,    4 },
    };
    Mesh* mesh = new Mesh(VertexFormat(elems, 2), 0, false);

    // cor por eixo
    Color color;
    if (direction.x > 0.9f)      color.Set(255, 0, 0, 255);
    else if (direction.y > 0.9f) color.Set(0, 255, 0, 255);
    else if (direction.z > 0.9f) color.Set(0, 0, 255, 255);
    else                         color.Set(255, 255, 255, 255);

    // usa direção normalizada para evitar escalas indesejadas
    const Vec3 axis = direction.normalized();

    const float shaftRadius = length * 0.015f;
    const float headRadius  = headSize * 0.40f;
    const float headLen     = headSize;
    const float shaftLen    = length - headLen;

    const int   segments    = 12;             // um pouco mais liso
    const float TWO_PI      = 6.28318530718f;

    // base ortonormal ao redor de 'axis'
    Vec3 up0 = (fabsf(axis.y) < 0.999f) ? Vec3(0,1,0) : Vec3(1,0,0);
    Vec3 right = Vec3::Cross(up0, axis).normalized();
    Vec3 up    = Vec3::Cross(axis, right).normalized();

    // ===== Shaft (cilindro fino, sem tampas) =====
    const int shaftStart = mesh->GetVertexCount();
    for (int ring = 0; ring <= 1; ++ring) {
        const float t = float(ring);
        const Vec3 center = axis * (t * shaftLen);
        for (int i = 0; i <= segments; ++i) {
            float ang = TWO_PI * (float)i / (float)segments;
            Vec3 radial = (right * cosf(ang) + up * sinf(ang));
            Vec3 pos = center + radial * shaftRadius;
            // como o formato é POS+COLOR, usa um AddVertex(pos,color). Se ainda não tens, cria esse overload.
            mesh->AddVertex(pos, color);
        }
    }
    const int vertsPerRing = segments + 1;
    for (int i = 0; i < segments; ++i) {
        int bl = shaftStart + i;
        int br = shaftStart + i + 1;
        int tl = shaftStart + vertsPerRing + i;
        int tr = shaftStart + vertsPerRing + i + 1;
        mesh->AddFace(bl, tl, br);
        mesh->AddFace(br, tl, tr);
    }

    // ===== Head (cone) =====
    const Vec3 tipPos  = axis * length;
    const Vec3 baseCtr = axis * shaftLen;

    // ponta
    const int tipIndex = mesh->GetVertexCount();
    mesh->AddVertex(tipPos, color);

    // anel da base do cone
    const int ringStart = mesh->GetVertexCount();
    for (int i = 0; i <= segments; ++i) {
        float ang = TWO_PI * (float)i / (float)segments;
        Vec3 radial = (right * cosf(ang) + up * sinf(ang));
        Vec3 pos = baseCtr + radial * headRadius;
        mesh->AddVertex(pos, color);
    }

    // laterais do cone (CCW visto de fora)
    for (int i = 0; i < segments; ++i) {
        int a = tipIndex;
        int b = ringStart + i;
        int c = ringStart + i + 1;
        mesh->AddFace(a, b, c);
    }

    // ===== FECHAR A CABEÇA (tampa da base do cone) =====
    // triangulação em leque (fan) com normal a apontar para TRÁS (−axis).
    const int baseCenterIdx = mesh->GetVertexCount();
    mesh->AddVertex(baseCtr, color);
    for (int i = 0; i < segments; ++i) {
        // ordem CCW quando vista “por trás” (lado de fora da tampa)
        int b = ringStart + i + 1;
        int c = ringStart + i;
        mesh->AddFace(baseCenterIdx, b, c);
    }

    mesh->Upload();
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    return mesh;
}



Mesh *MeshManager::CreateArrow(float length, float headSize, const std::string &name)
{
    VertexFormat::Element VertexElements[] = {
        VertexFormat::Element(VertexFormat::POSITION, 3),
        VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
        VertexFormat::Element(VertexFormat::NORMAL, 3),
        VertexFormat::Element(VertexFormat::TANGENT, 4),
    };
    
    Mesh *mesh = new Mesh(VertexFormat(VertexElements, 4), 0, false);
    
    const float shaftRadius = length * 0.02f;        // 2% da length para espessura do shaft
    const float headRadius = headSize * 0.5f;        // Raio da base da cabeça
    const float headLength = headSize;               // Comprimento da cabeça
    const float shaftLength = length - headLength;   // Comprimento do shaft
    
    const int segments = 8;  // Octógono para boa aparência
    const float PI = 3.14159265359f;
    const float PI2 = PI * 2.0f;
    
    // === SHAFT (CILINDRO) ===
    
    // Base do shaft (Y = 0)
    const int shaftBaseCenter = mesh->GetVertexCount();
    mesh->AddVertex(Vec3(0, 0, 0), Vec2(0.5f, 0.5f), Vec3(0, -1, 0));
    
    // Vértices da base do shaft
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = PI2 * segment / (float)segments;
        const float x = cosf(angle) * shaftRadius;
        const float z = sinf(angle) * shaftRadius;
        
        mesh->AddVertex(Vec3(x, 0, z), Vec2(0.5f + x/shaftRadius*0.5f, 0.5f + z/shaftRadius*0.5f), Vec3(0, -1, 0));
    }
    
    // Top do shaft (Y = shaftLength) 
    const int shaftTopCenter = mesh->GetVertexCount();
    mesh->AddVertex(Vec3(0, shaftLength, 0), Vec2(0.5f, 0.5f), Vec3(0, 1, 0));
    
    // Vértices do top do shaft
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = PI2 * segment / (float)segments;
        const float x = cosf(angle) * shaftRadius;
        const float z = sinf(angle) * shaftRadius;
        
        mesh->AddVertex(Vec3(x, shaftLength, z), Vec2(0.5f + x/shaftRadius*0.5f, 0.5f + z/shaftRadius*0.5f), Vec3(0, 1, 0));
    }
    
    // Corpo do shaft (vértices laterais)
    const int shaftSideStart = mesh->GetVertexCount();
    for (int ring = 0; ring <= 1; ++ring) {  // 0 = base, 1 = top
        const float y = ring * shaftLength;
        
        for (int segment = 0; segment <= segments; ++segment) {
            const float angle = PI2 * segment / (float)segments;
            const float x = cosf(angle) * shaftRadius;
            const float z = sinf(angle) * shaftRadius;
            
            Vec3 position(x, y, z);
            Vec3 normal(x / shaftRadius, 0, z / shaftRadius);  // Normal radial
            Vec2 uv((float)segment / segments, (float)ring);
            
            mesh->AddVertex(position, uv, normal);
        }
    }
    
    // === ARROW HEAD (CONE) ===
    
    // Tip da seta (Y = length)
    const int tipIndex = mesh->GetVertexCount();
    mesh->AddVertex(Vec3(0, length, 0), Vec2(0.5f, 1.0f), Vec3(0, 1, 0));
    
    // Base da head (Y = shaftLength)
    const int headBaseCenter = mesh->GetVertexCount();
    mesh->AddVertex(Vec3(0, shaftLength, 0), Vec2(0.5f, 0.5f), Vec3(0, -1, 0));
    
    // Vértices da base da head
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = PI2 * segment / (float)segments;
        const float x = cosf(angle) * headRadius;
        const float z = sinf(angle) * headRadius;
        
        mesh->AddVertex(Vec3(x, shaftLength, z), Vec2(0.5f + x/headRadius*0.5f, 0.5f + z/headRadius*0.5f), Vec3(0, -1, 0));
    }
    
    // Corpo da head (cone)
    const float headSlopeLength = sqrtf(headRadius * headRadius + headLength * headLength);
    const float normalY = headRadius / headSlopeLength;
    const float normalRadius = headLength / headSlopeLength;
    
    const int headSideStart = mesh->GetVertexCount();
    for (int segment = 0; segment <= segments; ++segment) {
        const float angle = PI2 * segment / (float)segments;
        const float x = cosf(angle) * headRadius;
        const float z = sinf(angle) * headRadius;
        
        Vec3 position(x, shaftLength, z);
        Vec3 normal((x / headRadius) * normalRadius, normalY, (z / headRadius) * normalRadius);
        Vec2 uv((float)segment / segments, 0.0f);
        
        mesh->AddVertex(position, uv, normal.normalized());
    }
    
    // === GERAR FACES ===
    
    // Base do shaft (triângulos)
    const int shaftBaseRing = shaftBaseCenter + 1;
    for (int segment = 0; segment < segments; ++segment) {
        const int current = shaftBaseRing + segment;
        const int next = shaftBaseRing + segment + 1;
        mesh->AddFace(shaftBaseCenter, next, current);  // CCW de baixo
    }
    
    // Top do shaft (triângulos)
    const int shaftTopRing = shaftTopCenter + 1;
    for (int segment = 0; segment < segments; ++segment) {
        const int current = shaftTopRing + segment;
        const int next = shaftTopRing + segment + 1;
        mesh->AddFace(shaftTopCenter, current, next);  // CCW de cima
    }
    
    // Lados do shaft (quads)
    const int vertsPerRing = segments + 1;
    for (int segment = 0; segment < segments; ++segment) {
        const int bottomLeft = shaftSideStart + segment;
        const int bottomRight = bottomLeft + 1;
        const int topLeft = bottomLeft + vertsPerRing;
        const int topRight = topLeft + 1;
        
        mesh->AddFace(bottomLeft, topLeft, bottomRight);
        mesh->AddFace(bottomRight, topLeft, topRight);
    }
    
    // Base da head (triângulos) 
    const int headBaseRing = headBaseCenter + 1;
    for (int segment = 0; segment < segments; ++segment) {
        const int current = headBaseRing + segment;
        const int next = headBaseRing + segment + 1;
        mesh->AddFace(headBaseCenter, next, current);  // CCW de baixo
    }
    
    // Lados da head (triângulos do tip para a base)
    for (int segment = 0; segment < segments; ++segment) {
        const int current = headSideStart + segment;
        const int next = headSideStart + segment + 1;
        mesh->AddFace(tipIndex, current, next);  // CCW de fora
    }
    
    mesh->CalculateTangents();
    mesh->Upload();
    
    m_meshes.push_back(mesh);
    m_meshesByName[name] = mesh;
    
    return mesh;
}