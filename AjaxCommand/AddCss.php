<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/17/16
 * Time: 8:26 AM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;

/**
 * Add Css wrapper commands
 * 
 * @FrxAjaxCommand(
 *   id = "add_css"
 * )
 */
class AddCss implements AjaxCommandInterface {

  public function commandFromSettings(array $settings) {
    $text = $settings['text'];
    return ajax_command_add_css($text); 
  }
}