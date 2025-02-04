/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bsplevel.h
 * @author Brian Lach
 * @date September 09, 2020
 */

#pragma once

#include "config_bsp.h"
#include "referenceCount.h"
#include "nodePath.h"
#include "renderAttrib.h"
#include "lightmap_palettes.h"
#include "ambient_probes.h"
#include "cubemaps.h"
#include "rigidBodyCombiner.h"
#include "decals.h"
#include "bsp_trace.h"
#include "eggVertex.h"
#include "bspMaterial.h"
#include "bulletRigidBodyNode.h"

class BulletWorld;
class BSPLoader;
class EggVertexPool;
class EggPolygon;
class GraphicsOutput;

struct texinfo_s;
struct dvertex_s;
struct texref_s;
struct dface_s;
struct dedge_s;
struct entity_s;
typedef texinfo_s texinfo_t;
typedef dvertex_s dvertex_t;
typedef texref_s texref_t;
typedef dface_s dface_t;
typedef dedge_s dedge_t;
typedef entity_s entity_t;
struct bspdata_t;
#ifdef CPPPARSER
struct dmodel_t;
#endif

/**
 * An attribute applied to each face Geom from a BSP file.
 * All it does right now is indicate the material of the face
 * and if it's a wall or a floor (depending on the face normal).
 */
class EXPCL_PANDABSP BSPFaceAttrib : public RenderAttrib {
private:
  INLINE BSPFaceAttrib() :
    RenderAttrib(),
    _ignore_pvs(false) {
  }

PUBLISHED:
  enum {
    FACETYPE_WALL,
    FACETYPE_FLOOR,
  };

  static CPT(RenderAttrib) make(const std::string &face_material, int face_type);
  static CPT(RenderAttrib) make_default();
  static CPT(RenderAttrib) make_ignore_pvs();

  INLINE std::string get_material() const {
    return _material;
  }
  INLINE int get_face_type() const {
    return _face_type;
  }
  INLINE bool get_ignore_pvs() const {
    return _ignore_pvs;
  }

public:
  virtual bool has_cull_callback() const;

  virtual size_t get_hash_impl() const;
  virtual int compare_to_impl(const RenderAttrib *other) const;

private:
  std::string _material;
  int _face_type;
  bool _ignore_pvs;

PUBLISHED:
  static int get_class_slot() {
    return _attrib_slot;
  }
  virtual int get_slot() const {
    return get_class_slot();
  }
  MAKE_PROPERTY(class_slot, get_class_slot);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    RenderAttrib::init_type();
    register_type(_type_handle, "BSPFaceAttrib",
                  RenderAttrib::get_class_type());
    _attrib_slot = register_slot(_type_handle, 100, new BSPFaceAttrib);
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {
    init_type(); return get_class_type();
  }

private:
  static TypeHandle _type_handle;
  static int _attrib_slot;

};

struct collbspdata_t;

struct dface_lightmap_info_t {
  float s_scale, t_scale;
  float s_offset, t_offset;
  int texmins[2];
  int texsize[2];
  LightmapPalette::Entry *palette_entry;
};

struct brush_collision_data_t {
  std::string material;
  int modelnum;
};

struct brush_model_data_t {
  int modelnum;
  int merged_modelnum;
  LPoint3 origin;
  LMatrix4f origin_matrix;
  NodePath model_root;
  PT(RigidBodyCombiner) decal_rbc;

  brush_model_data_t() {
    decal_rbc = new RigidBodyCombiner("decal-rbc");
  }
};

/**
 * An instance of a loaded BSP level.
 */
class EXPCL_PANDABSP BSPLevel : public ReferenceCount {
PUBLISHED:

  void remove_physics(const NodePath &root);
  BulletWorld *get_physics_world() const;

  INLINE bool has_brush_collision_node(BulletRigidBodyNode *rbnode) const {
    return _brush_collision_data.find(rbnode) != _brush_collision_data.end();
  }

  INLINE bool has_brush_collision_triangle(BulletRigidBodyNode *rbnode, int triangle_idx) {
    return _brush_collision_data[rbnode].find(triangle_idx) != _brush_collision_data[rbnode].end();
  }

  INLINE std::string get_brush_triangle_material(BulletRigidBodyNode *rbnode, int triangle_idx) {
    return _brush_collision_data[rbnode][triangle_idx].material;
  }

  INLINE int get_brush_triangle_model(BulletRigidBodyNode *rbnode, int triangle_idx) {
    return _brush_collision_data[rbnode][triangle_idx].modelnum;
  }

  int get_brush_triangle_model_fast(BulletRigidBodyNode *rbnode, int triangle_idx);

  INLINE LPoint3 get_model_origin(int modelnum) {
    return _model_data[modelnum].origin;
  }

  INLINE void trace_decal(const std::string &decal_material, const LPoint2 &decal_scale,
                          float rotate, const LPoint3 &start, const LPoint3 &end, const LColorf &decal_color = LColorf(1),
                          int flags = 0) {
    _decal_mgr.decal_trace(decal_material, decal_scale, rotate, start, end, decal_color, flags);
  }

  Texture *get_closest_cubemap_texture(const LPoint3 &pos);

  int extract_modelnum(int entnum);
  void get_model_bounds(int modelnum, LPoint3 &mins, LPoint3 &maxs);

  bool trace_line(const LPoint3 &start, const LPoint3 &end);
  LPoint3 clip_line(const LPoint3 &start, const LPoint3 &end);

  NodePath get_model(int modelnum) const;

  INLINE int find_leaf(const NodePath &np) {
    return find_leaf(np.get_pos(_result));
  }

  int find_leaf(const LPoint3 &pos, int headnode = 0);
  int find_node(const LPoint3 &pos);
  bool is_cluster_visible(int curr_cluster, int cluster) const;

  bool pvs_bounds_test(const GeometricBoundingVolume *bounds, unsigned int required_leaf_flags = 0u);
  CPT(GeometricBoundingVolume) make_net_bounds(const TransformState *net_transform,
                                               const GeometricBoundingVolume *original);

  INLINE NodePath get_result() const {
    return _result;
  }

  INLINE void set_current_leaf(int leaf) {
    update_leaf(leaf);
  }
  INLINE int get_current_leaf() const {
    return _curr_leaf_idx;
  }
  INLINE int get_num_visleafs() const {
    return _bspdata->dmodels[0].visleafs + 1;
  }
  INLINE LPoint3 get_leaf_center(int leaf) const {
    BoundingBox *bbox = _leaf_bboxs[leaf];
    return bbox->get_approx_center();
  }

  LTexCoord get_lightcoords(int facenum, const LVector3 &point);

  void do_optimizations();

  void build_cubemaps(const NodePath &render, GraphicsOutput *window);

  BSPLoader *get_loader() const {
    return _loader;
  }

public:
  INLINE brush_model_data_t &get_brush_model_data(int modelnum) {
    return _model_data[modelnum];
  }

  INLINE bspdata_t *get_bspdata() const {
    return _bspdata;
  }
  INLINE collbspdata_t *get_colldata() const {
    return _colldata;
  }
  INLINE entity_t *get_light_environment() const {
    return _light_environment;
  }
  INLINE AmbientProbeManager *get_ambient_probe_mgr() {
    return &_amb_probe_mgr;
  }
  INLINE BSPTrace *get_trace() const {
    return _trace;
  }
  INLINE LightmapPaletteDirectory *get_lightmap_dir() {
    return _lightmap_dir;
  }

  INLINE const dmodel_t *dmodel_for_dface(const dface_t *dface) const {
    auto itr = _dface_dmodels.find(dface);
    if (itr != _dface_dmodels.end())
      return itr->second;

    return nullptr;
  }

  void update_visibility(const LPoint3 &pos);

  void remove_model(int modelnum);

  INLINE void set_light_environment(entity_t *ent) {
    _light_environment = ent;
  }

  static int extract_modelnum_s(entity_t *ent);
  static void flatten_node(const NodePath &node);
  static void clear_model_nodes_below(const NodePath &top);

protected:
  BSPLevel(BSPLoader *loader);

  void set_filename(const Filename &file);

  virtual bool load(bspdata_t *bspdata);
  virtual void cleanup(bool is_transition);

  virtual void load_geometry() = 0;

  void setup_raytrace_environment();

  void update_leaf(int leaf);

  void make_faces();

  void make_faces_ai();
  NodePath make_faces_ai_base(const std::string &name, const vector_string &include_entities,
                              const vector_string &exclude_entities = vector_string());
  NodePath make_model_faces(int modelnum);

  void make_brush_model_collisions(int modelnum = -1);

  void load_static_props();
  void load_cubemaps();

  LTexCoord get_vertex_uv(texinfo_t *texinfo, dvertex_t *vert, bool lightmap = false) const;

  PT(EggVertex) make_vertex(EggVertexPool *vpool, EggPolygon *poly,
                            dedge_t *edge, texinfo_t *texinfo,
                            dface_t *face, int k, Texture *tex);
  PT(EggVertex) make_vertex_ai(EggVertexPool *vpool, EggPolygon *poly, dedge_t *edge, int k);

  cubemap_t *find_closest_cubemap(const LPoint3 &pos);

  void init_dface_lightmap_info(dface_lightmap_info_t *info, int facenum);

protected:
  BSPLoader *_loader;

  bspdata_t *_bspdata;
  collbspdata_t *_colldata;

  NodePath _result;

  NodePath _fake_dl;
  LVector3 _shadow_dir;
  entity_t *_light_environment;

  bool _has_pvs_data;

  PT(BSPTrace) _trace;

  struct visibleleafdata_t {
    BoundingBox *bbox;
    int flags;
  };
  pvector<visibleleafdata_t> _visible_leaf_bboxs;
  pvector<int> _visible_leafs;
  int _curr_leaf_idx;

  Filename _map_file;

  std::unordered_map<const dface_t *, const dmodel_t *> _dface_dmodels;

  pmap<texref_t *, CPT(BSPMaterial)> _texref_materials;

  pvector<uint8_t *> _leaf_pvs;
  pvector<NodePath> _leaf_visnp;
  pvector<PT(BoundingBox)> _leaf_bboxs;

  pvector<brush_model_data_t> _model_data;

  PT(LightmapPaletteDirectory) _lightmap_dir;
  pvector<dface_lightmap_info_t> _face_lightmap_info;

  AmbientProbeManager _amb_probe_mgr;

  DecalManager _decal_mgr;

  pvector<cubemap_t> _cubemaps;

  typedef unordered_map<int, brush_collision_data_t> TriangleIndex2BSPCollisionData_t;
  typedef pmap<PT(BulletRigidBodyNode), TriangleIndex2BSPCollisionData_t> BSPCollisionData_t;
  BSPCollisionData_t _brush_collision_data;

  // A per-leaf list of world Geoms.
  // This list of Geoms will be rendered for the current leaf.
  pvector<GeomNode::Geoms> _leaf_world_geoms;

  friend class BSPLoader;
  friend class BSPCullTraverser;
};

INLINE void BSPLevel::
set_filename(const Filename &file) {
  _map_file = file;
}
