<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/17/16
 * Time: 5:58 PM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;


/**
 * Class Restripe
 * 
 * @FrxAjaxCommand(
 *   id = "restripe"
 * )
 */
class Restripe extends AjaxCommandBase {
  public function commandFromSettings(array $settings) {
    $selector = $this->commandFromSettings($settings, 'selector');
    return ajax_command_restripe($selector); 
  }
}