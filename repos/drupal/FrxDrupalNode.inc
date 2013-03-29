<?php

class FrxDrupalNode {
  public $access = 'access content';

  public function load($parms = array()) {
    GLOBAL $language;
    $node = new SimpleXMLElement('<node/>');
    $nid = isset($parms['nid']) ? $parms['nid']: 1;
    $vid = isset($parms['vid']) ? $parms['vid']: NULL;
    $data = node_load($nid, $vid);
    $lang = isset($data->language) ? $data->language : $language->language;
    
    foreach ($data as $key => $val) {
      if(is_array($val) && isset($val[$lang])) {
        $tmp = $val[$lang][0];
        $child = $node->addChild($key, $tmp['value']);
        if (isset($tmp['format'])) {
          $child->addAttribute('format', $tmp['format']);
        }
        if (isset($tmp['data_type'])) {
          $child->addAttribute('data_type', $tmp['data_type']);
        }
      } else if (is_string($val)) {
        $node->addChild($key, $val);
      }
    }
    return $node;
  }
}