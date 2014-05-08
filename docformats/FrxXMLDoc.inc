<?php
/**
 * @file FrxHtmlDoc
 * Straight XML document with no wrapping theme.
 * @author davidmetzler
 *
 */
class FrxXMLDoc extends FrxDocument {
  public $root_tag = 'div';
  public $root_attributes = array();

  public function __construct() {
    $this->content_type = 'application/xml';
    $skin = Frx::Data()->getContext('skin');
    if (isset($skin['FrxXMLDoc']['rootElementName'])) {
      $this->root_tag = $skin['FrxXMLDoc']['rootElementName'];
    }
    if (isset($skin['FrxXMLDoc']['rootElementAttributes'])) {
      if (is_array($skin['FrxXMLDoc']['rootElementAttributes'])) {
        $this->root_attributes = $skin['FrxXMLDoc']['rootElementAttributes'];
      }
    }
  }

  public function render($r, $format, $options = array()) {
    $body = $this->check_markup($r->html);
    $output = '';
    $opening_tag = $this->_build_opening_root_tag_contents();
    $ending_tag = $this->root_tag;
    $output .= "<$opening_tag>\n  $body\n</$ending_tag>";
    return $output;
  }

  public function output($output) {
      header('Content-Type: ' . $this->content_type);
      header('Cache-Control:');
      header('Pragma:');
      header('Cache-Control: must-revalidate');
      $output = '<?xml version="1.0"?>' . "\n" . $output;
      print  $output;
      return TRUE;
  }

  private function _build_opening_root_tag_contents() {
    $tag_contents = $this->root_tag;
    if (isset($this->root_attributes) && is_array($this->root_attributes)) {
      foreach ($this->root_attributes as $attr_key => $attr_val) {
        if (!empty($attr_key) && (string)$attr_val !=='') {
          $attr_val = addslashes($attr_val);
          $tag_contents .= " " . $attr_key . "=" . "'$attr_val'";
        }
      }
    }
    return $tag_contents;
  }
}
