#include "MyColorDimmerSwitch.h"

#if CONFIG_ZB_ENABLED

MyColorDimmerSwitch::MyColorDimmerSwitch(uint8_t endpoint)
    : ZigbeeColorDimmerSwitch(endpoint) {
  // Add Scenes cluster (client role) so the endpoint can legally send Scenes commands.
  // This must happen after the base class has created `_cluster_list`.
  if (_cluster_list != nullptr) {
    esp_zb_cluster_list_add_scenes_cluster(
        _cluster_list,
        esp_zb_scenes_cluster_create(nullptr),
        ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  }
}

void MyColorDimmerSwitch::recallScene(uint8_t scene_id) {
  esp_zb_zcl_scenes_recall_scene_cmd_t recall_scene_cmd;
  memset(&recall_scene_cmd, 0, sizeof(recall_scene_cmd));

  recall_scene_cmd.zcl_basic_cmd.src_endpoint = getEndpoint();
  // Send using bindings (manual binding is typically configured in HA/Z2M).
  recall_scene_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
  // Non-group scenes: group_id must be 0x0000.
  recall_scene_cmd.group_id = 0x0000;
  recall_scene_cmd.scene_id = scene_id;

  esp_zb_lock_acquire(portMAX_DELAY);
  esp_zb_zcl_scenes_recall_scene_cmd_req(&recall_scene_cmd);
  esp_zb_lock_release();
}

#else

MyColorDimmerSwitch::MyColorDimmerSwitch(uint8_t endpoint)
    : ZigbeeColorDimmerSwitch(endpoint) {}

void MyColorDimmerSwitch::recallScene(uint8_t /*scene_id*/) {}

#endif
