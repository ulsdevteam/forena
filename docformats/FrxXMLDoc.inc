<?php
/**
 * @file FrxHtmlDoc
 * Straight XML document with no wrapping theme.
 * @author davidmetzler
 *
 */
class FrxXMLDoc extends FrxDocument {

  public function __construct() {
    $this->content_type = 'application/xml';
  }

  public function render($r, $format, $options = array()) {
    $body = $this->check_markup($r->html);
    $output = '<?xml version="1.0"?>' . "\n";
    $output .= '<div>' . $body . "</div>\n";
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