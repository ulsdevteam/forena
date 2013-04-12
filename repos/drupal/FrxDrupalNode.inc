<?php
/**
 * Implementation of a drupal node load function
 * @author baileys
 */
class FrxDrupalNode {
  public $access = 'access content';
  public $tokens = array('nid', 'vid');

  public function load($parms = array()) {
    $nid = isset($parms['nid']) ? $parms['nid'] : 1;
    $vid = isset($parms['vid']) ? $parms['vid'] : NULL;
    $node = node_load($nid, $vid);
    $return = new SimpleXMLElement('<node/>');
    $lang = isset($node->language) ? $node->language : 'unk';

    if ($node) foreach ($node as $key => $val) {
      if (strpos($key, 'field_') === 0) {
        $fields = field_get_items('node', $node, $key);
        $output = @field_view_value('node', $node, $key, $fields[0]);
        $return->addChild($key, drupal_render($output));
      } else if (is_array($val) && isset($val[$lang])) {
        $tmp = $val[$lang][0];
        if (isset($tmp['safe_value'])) {
          $return->addChild($key, $tmp['safe_value']);
        } else if (isset($tmp['value'])) {
          $return->addChild($key, $tmp['value']);
        }
      } else if (is_scalar($val)) {
        $return->addChild($key, $val);
      }
    }
    return $return;
  }
}