<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/17/16
 * Time: 5:24 PM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;


/**
 * Class Data
 * 
 * @FrxAjaxCommand(
 *   id = "data"
 * )
 */
class Data extends AjaxCommandBase{
  public function commandFromSettings(array $settings) {
    $selector = $this->getSetting($settings, 'selector');
    $name = $this->getSetting($settings, 'name');
    $value = $this->getSetting($selector, 'value'); 
    return ajax_command_data($selector, $name, $value); 
  }
}