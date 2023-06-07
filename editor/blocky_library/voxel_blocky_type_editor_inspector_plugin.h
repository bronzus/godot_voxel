#ifndef VOXEL_BLOCKY_TYPE_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_BLOCKY_TYPE_EDITOR_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"

namespace zylann::voxel {

class VoxelBlockyTypeEditorInspectorPlugin : public ZN_EditorInspectorPlugin {
	GDCLASS(VoxelBlockyTypeEditorInspectorPlugin, ZN_EditorInspectorPlugin)
protected:
	bool _zn_can_handle(const Object *p_object) const override;
	void _zn_parse_begin(Object *p_object) override;
	bool _zn_parse_property(Object *p_object, const Variant::Type p_type, const String &p_path,
			const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage,
			const bool p_wide) override;

private:
	// When compiling with GodotCpp, `_bind_methods` isn't optional.
	static void _bind_methods() {}
};

} // namespace zylann::voxel

#endif // VOXEL_BLOCKY_TYPE_EDITOR_INSPECTOR_PLUGIN_H