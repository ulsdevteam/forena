<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/17/16
 * Time: 6:03 PM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;


/**
 * Class Settings
 * 
 * @FrxAjaxCommand(
 *   id = "settings"
 * )
 */
class Settings extends AjaxCommandBase {
  public function commandFromSettings(array $settings) {
    if (!isset($settings['merge'])) {
      $settings['merge'] = TRUE;
    }
    $merge = $this->getSetting($settings, 'merge');
    $json_settings = $this->getJSONText($settings, 'settings');
    return ajax_command_settings($json_settings, $merge); 
  }
}