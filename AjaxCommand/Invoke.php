<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/16/16
 * Time: 2:27 PM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;

/**
 * Class Invoke
 * 
 * @FrxAjaxCommand(
 *   id = "invoke"
 * )
 */
class Invoke implements AjaxCommandInterface{

  /**
   * {@inheritdoc}
   */
  public function commandFromSettings(array $settings) {
    $selector = $settings['selector']; 
    $method = $settings['method'];
    $arguments = []; 
    if (isset($settings['arguments'])) {
      $arguments = $settings['arguments'];
      if (!is_array($arguments)) $arguments = json_decode($arguments);
    }
    elseif (isset($settings['text'])) {
      $arguments = json_decode($settings['text']);
    }
    if (!$arguments) $arguments = array();
    $command = ajax_command_invoke($selector, $method, $arguments); 
    return $command; 
  }
}