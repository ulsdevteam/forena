<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/17/16
 * Time: 8:34 AM
 */

namespace Drupal\forena\FrxPlugin\AjaxCommand;


use Drupal\forena\Token\ReportReplacer;

abstract class AjaxCommandBase implements AjaxCommandInterface {

  private $replacer;

  /**
   * AjaxCommandBase constructor.
   * @param \FrxSyntaxEngine $replacer
   *   Token replacer
   */
  public function __construct($replacer) {
    $this->replacer = $replacer; 
  }

  /**
   * @param $settings
   * @param $key
   * @return mixed
   *   The settings in the array
   */
  public function getSetting(&$settings, $key) {
    $value = NULL;
    if (isset($settings[$key])) {
      $value = $settings[$key];
      unset($settings[$key]);
    }
    $this->replacer->replaceNested($value);
    return $value;
  }

  /**
   * @param array $settings
   * @param string $default_key
   *   The key to look for if the text
   *
   */
  public function getJSONText(&$settings, $default_key='') {
    $data = [];
    if ($default_key) $data = $this->getSetting($default_key);
    if (!empty($settings['text'])) {
      $data = $settings['text'];
      $data = @json_decode($data);
      unset($settings['text']);
      $this->replacer->replaceNested($data);
    }
    return $data;
  }


}