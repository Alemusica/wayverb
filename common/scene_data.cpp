#include "scene_data.h"

#include "boundaries.h"
#include "conversions.h"

#include "config.h"

#include <vector>
#include <stdexcept>
#include <fstream>

TriangleVec3f get_triangle_verts(const Triangle& t,
                                 const std::vector<Vec3f>& v) {
    return TriangleVec3f{{v[t.v0], v[t.v1], v[t.v2]}};
}

TriangleVec3f get_triangle_verts(const Triangle& t,
                                 const std::vector<cl_float3>& v) {
    return TriangleVec3f{
        {to_vec3f(v[t.v0]), to_vec3f(v[t.v1]), to_vec3f(v[t.v2])}};
}

void attemptJsonParse(const std::string& fname, rapidjson::Document& doc) {
    std::ifstream in(fname);
    std::string file((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());

    doc.Parse(file.c_str());
}

SurfaceLoader::SurfaceLoader(const std::string& fpath) {
    Surface defaultSurface{
        VolumeType({{0.92, 0.92, 0.93, 0.93, 0.94, 0.95, 0.95, 0.95}}),
        VolumeType({{0.50, 0.90, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95}})};

    add_surface("default", defaultSurface);

    rapidjson::Document document;
    attemptJsonParse(fpath, document);
    if (!document.IsObject())
        throw std::runtime_error("Materials must be stored in a JSON object");

    for (auto i = document.MemberBegin(); i != document.MemberEnd(); ++i) {
        std::string name = i->name.GetString();

        Surface surface;
        ValueJsonValidator<Surface>(surface).run(i->value);

        add_surface(name, surface);
    }
}

void SurfaceLoader::add_surface(const std::string& name,
                                const Surface& surface) {
    surfaces.push_back(surface);
    surface_indices[name] = surfaces.size() - 1;
}

std::vector<Surface> SurfaceLoader::get_surfaces() const {
    return surfaces;
}

SurfaceLoader::size_type SurfaceLoader::get_index(
    const std::string& name) const {
    auto ret = 0;
    auto it = surface_indices.find(name);
    if (it != surface_indices.end())
        ret = it->second;
    return ret;
}

SceneData::SceneData(const std::string& fpath,
                     const std::string& mat_file,
                     float scale) {
    populate(fpath, mat_file, scale);
}

SceneData::SceneData(const aiScene* const scene,
                     const std::string& mat_file,
                     float scale) {
    populate(scene, mat_file, scale);
}

void SceneData::populate(const aiScene* const scene,
                         const std::string& mat_file,
                         float scale) {
    if (!scene)
        throw std::runtime_error("scene pointer is null");

    SurfaceLoader surface_loader(mat_file);
    surfaces = surface_loader.get_surfaces();

    triangles.clear();
    vertices.clear();

    for (auto i = 0u; i != scene->mNumMeshes; ++i) {
        auto mesh = scene->mMeshes[i];

        auto material = scene->mMaterials[mesh->mMaterialIndex];
        aiString matName;
        material->Get(AI_MATKEY_NAME, matName);

        auto surface_index = surface_loader.get_index(matName.C_Str());

        std::vector<cl_float3> meshVertices(mesh->mNumVertices);

        for (auto j = 0u; j != mesh->mNumVertices; ++j) {
            meshVertices[j] = to_cl_float3(mesh->mVertices[j]);
        }

        std::vector<Triangle> meshTriangles(mesh->mNumFaces);

        for (auto j = 0u; j != mesh->mNumFaces; ++j) {
            auto face = mesh->mFaces[j];

            meshTriangles[j] = Triangle{surface_index,
                                        face.mIndices[0] + vertices.size(),
                                        face.mIndices[1] + vertices.size(),
                                        face.mIndices[2] + vertices.size()};
        }

        vertices.insert(
            vertices.end(), meshVertices.begin(), meshVertices.end());
        triangles.insert(
            triangles.end(), meshTriangles.begin(), meshTriangles.end());
    }

    std::for_each(vertices.begin(),
                  vertices.end(),
                  [scale](auto& i) {
                      std::for_each(std::begin(i.s),
                                    std::end(i.s),
                                    [scale](auto& i) { i *= scale; });
                  });
}

void SceneData::populate(const std::string& fpath,
                         const std::string& mat_file,
                         float scale) {
    Assimp::Importer importer;
    try {
        populate(
            importer.ReadFile(fpath,
                              (aiProcess_Triangulate |
                               aiProcess_GenSmoothNormals | aiProcess_FlipUVs)),
            mat_file,
            scale);
    } catch (...) {
        throw std::runtime_error("failed to read file");
    }
}

CuboidBoundary SceneData::get_aabb() const {
    return get_cuboid_boundary(get_converted_vertices());
}

std::vector<Vec3f> SceneData::get_converted_vertices() const {
    std::vector<Vec3f> vec(vertices.size());
    std::transform(vertices.begin(),
                   vertices.end(),
                   vec.begin(),
                   [](auto i) { return to_vec3f(i); });
    return vec;
}

std::vector<int> SceneData::get_triangle_indices() const {
    std::vector<int> ret(triangles.size());
    std::iota(ret.begin(), ret.end(), 0);
    return ret;
}

const std::vector<Triangle>& SceneData::get_triangles() const {
    return triangles;
}
const std::vector<cl_float3>& SceneData::get_vertices() const {
    return vertices;
}
const std::vector<Surface>& SceneData::get_surfaces() const {
    return surfaces;
}
