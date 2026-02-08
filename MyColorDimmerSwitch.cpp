#include "MyColorDimmerSwitch.h"

MyColorDimmerSwitch::MyColorDimmerSwitch(uint8_t endpoint)
    : PatchedZigbeeColorDimmerSwitch(endpoint) {
  
  _on_light_scene_change = nullptr;
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

void MyColorDimmerSwitch::startDimming(bool up, uint8_t rate) {
  esp_zb_zcl_level_move_cmd_t move_cmd;
  memset(&move_cmd, 0, sizeof(move_cmd));

  move_cmd.zcl_basic_cmd.src_endpoint = getEndpoint();
  move_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
  move_cmd.move_mode = up ? 0x00 : 0x01;
  move_cmd.rate = rate;

  esp_zb_lock_acquire(portMAX_DELAY);
  esp_zb_zcl_level_move_cmd_req(&move_cmd);
  esp_zb_lock_release();
}

void MyColorDimmerSwitch::stopDimming() {
  esp_zb_zcl_level_stop_cmd_t stop_cmd;
  memset(&stop_cmd, 0, sizeof(stop_cmd));

  stop_cmd.zcl_basic_cmd.src_endpoint = getEndpoint();
  stop_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;

  esp_zb_lock_acquire(portMAX_DELAY);
  esp_zb_zcl_level_stop_cmd_req(&stop_cmd);
  esp_zb_lock_release();
}

void MyColorDimmerSwitch::zbAttributeRead(
    uint16_t cluster_id, const esp_zb_zcl_attribute_t *attribute,
    uint8_t src_endpoint, esp_zb_zcl_addr_t src_address) {
  // Keep the base class behavior (reporting/handling) intact.
  PatchedZigbeeColorDimmerSwitch::zbAttributeRead(cluster_id, attribute, src_endpoint, src_address);

  // Custom handling can be added here.
  if (cluster_id == ESP_ZB_ZCL_CLUSTER_ID_SCENES) {
    Serial.printf("Scene attribute read: id=0x%04X type=0x%02X\r\n", attribute->id, attribute->data.type);
    if (attribute->id == ESP_ZB_ZCL_ATTR_SCENES_CURRENT_SCENE_ID && attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_U8) {
      uint8_t light_scene = attribute->data.value ? *(uint8_t *)attribute->data.value : 0;
      if (_on_light_scene_change) {
        _on_light_scene_change(light_scene);
      }
    }
  }
}
