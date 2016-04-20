<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/17/16
 * Time: 8:30 AM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;


/**
 * Class After
 * 
 * @FrxAjaxCommand(
 *   id = "after"
 * )
 */
class After extends AjaxCommandBase {

  /**
   * {@inheritdoc}
   */
  public function commandFromSettings(array $settings) {
    $selector = $this->getSetting($settings, 'selector');
    $text = $this->getSetting($settings, 'text');
    return ajax_command_after($selector, $text, $settings); 
  }
}