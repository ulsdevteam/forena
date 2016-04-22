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
class Invoke extends AjaxCommandBase {

  /**
   * {@inheritdoc}
   */
  public function commandFromSettings(array $settings) {
    $selector = $settings['selector'];
    $method = $settings['method'];
    $arguments = $this->getJSONText($settings, 'arguments');
    if (!$arguments) $arguments = array();
    $command = ajax_command_invoke($selector, $method, $arguments); 
    return $command; 
  }
}