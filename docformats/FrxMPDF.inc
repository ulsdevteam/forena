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

    // To do - use config variable of path to mpdf libs
    define('_MPDF_PATH', 'sites/all/libraries/mpdf/');
    include_once('sites/all/libraries/mpdf/mpdf.php');
    $this->content_type='application/pdf';

  }


  public function render($r, $format, $options = array()) {
// To Do
// The option switch off links on PDF will bee good here too

    $mpdf = new mPDF('UTF-8');
    $mpdf->AddPageByArray(array(
      'orientation' => 'P'
    ));

    $output = '';
    $output = '<html><head>';
    $output .= '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>';
    if (@$options['css']) {
     $output .= '<style type="text/css">';
     $output .= $css;
     $output .= '</style>';
    }
    $output .= '<title>' . $r->title . "</title></head><body class='forena-report $link_class'><h1>" . $this->check_markup($r->title) . '</h1>' . $r->html;
    $output .= '</body></html>';

    foreach (Frx::Skin()->stylesheets as $type => $sheets) {
      foreach ($sheets as $sheet) {
        switch ($type) {
           case 'all':
           case 'print':
           case 'screen':
           case 'pdf':
             $mpdf->WriteHTML(file_get_contents($sheet), 1);
           //echo $sheet;
           break;
        }
      }
    }

    $mpdf->WriteHTML($output);
    // $pdf = $mpdf->Output('', 'S');
    $pdf = $mpdf->Output();
    return $pdf;
  }


  public function output($pdf) {

    $http_headers = array(
      'Pragma' => 'no-cache',
      'Expires' => '0',
      'Cache-Control' => 'no-cache, must-revalidate',
      'Cache-Control' => 'private',
      'Content-Transfer-Encoding' => 'binary',
      'Content-Type' => 'application/pdf',
      // ! TODO !
      // after testing beter to put disable this coments to offer save file or take a config variable that chose to save or open.
      // 'Content-Length' => strlen($pdf),
      // 'Content-Disposition' => 'attachment; filename="a_report_name.pdf"',
    );

    foreach ($http_headers as $name => $value) {
      $value = preg_replace('/\r?\n(?!\t| )/', '', $value);
      drupal_add_http_header($name, $value);
    }
    return TRUE;
  }
}
