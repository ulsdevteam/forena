<?php
/**
 * @file FrxMPDF.inc
 * PDF document via MPDF Library
 * @author davidmetzler
 *
 */
class FrxMPDF extends FrxDocument {
  private $p;

  public function __construct() {
    include_once('sites/all/libraries/mpdf/mpdf.php');
    $this->content_type='application/pdf';

  }


  public function render($r, $format, $options = array()) {

    $output = '<html><head>';
    $output .= '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>';
    if (@$options['css']) {
      $output .= '<style type="text/css">';
      $output .= $css;
      $output .= '</style>';
    }

    $output .= '<title>' . $r->title . "</title></head><body class='forena-report $link_class'><h1>" . $r->title . '</h1>' . $r->html . '</body></html>';

    if (!class_exists('mPDF')) {
      drupal+set_message(t('MPDF Library not installed corectly.'), 'error');
      return '';
    }
    //@TODO: take the html document in $ouptut and convert it into a PDF document.
    // the resulting pdf document data should be returned by this function.
    return $output;
  }


  public function output($output) {
  //    header('Content-Type: ' . $this->content_type);
  //    header('Cache-Control:');
  //    header('Pragma:');
  //    header('Cache-Control: must-revalidate');
  //
    echo 'Generating pdf output';
    echo $output;
    return TRUE;
  }
}
