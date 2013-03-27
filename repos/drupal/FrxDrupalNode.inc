<?php

class FrxDrupalNode {
  public $access = 'access content';

  public function load($parms = array()) {
    GLOBAL $language;
    $node = new SimpleXMLElement('<node/>');
    $nid = isset($parms['nid']) ? $parms['nid']: 1;
    $vid = isset($parms['vid']) ? $parms['vid']: NULL;
    $lang = isset($parms['language'])  ? $parms['language'] : $language->language;
    $data = node_load($nid, $vid);
    //dpm($data);
    foreach($data as $key => $val) {
      if(is_array($val) && array_key_exists('und', $val)) {
        $node->addChild($key, $val[$lang][0]['value']);
      } else if (is_string($val)) {
        $node->addChild($key, $val);
      }
    }
    return $node;
  }
}