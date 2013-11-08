<?php
/**
 * @file FrxHtmlDoc
 * Straight XML document with no wrapping theme.
 * @author davidmetzler
 *
 */
class FrxXMLDoc extends FrxDocument {
  public $root_tag = 'div';

  public function __construct() {
    $this->content_type = 'application/xml';
    $skin = Frx::Data()->getContext('skin');
    if (isset($skin['FrxXMLDoc']['rootElementName'])) {
      $this->root_tag = $skin['FrxXMLDoc']['rootElementName'];
    }
  }

  public function render($r, $format, $options = array()) {
    $body = $this->check_markup($r->html);
    $tag = $this->root_tag;
    $output = '<?xml version="1.0"?>' . "\n";
    $output .= "<$tag>\n  $body\n</$tag>";
    return $output;
  }

  public function output($output) {
      header('Content-Type: ' . $this->content_type);
      header('Cache-Control:');
      header('Pragma:');
      header('Cache-Control: must-revalidate');
      print  $output;
      return TRUE;
      }
}