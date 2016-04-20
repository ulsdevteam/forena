<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/17/16
 * Time: 4:36 PM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;



/**
 * Class Changed
 * 
 * @FrxAjaxCommand(
 *   id = "changed"
 * )
 */
class Changed extends AjaxCommandBase {

  /**
   * Changed command
   * 
   * Propierties
   * 
   * @param array $settings
   * @return array
   *
   */
  public function commandFromSettings(array $settings) {
    $selector = $this->getSetting($settings, 'selector'); 
    $asterisk = $this->getSetting($settings, 'asterisk'); 
    return ajax_command_changed($selector, $asterisk); 
  }
}