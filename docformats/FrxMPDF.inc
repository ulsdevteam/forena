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

    $disable_links = variable_get('forena_pdf_disable_links', TRUE);
    $html = $this->check_markup($r->html);
    if ($disable_links) {
      $html = preg_replace('/<a href=\"(.*?)\">(.*?)<\/a>/', "\\2", $html);
    }

    $mpdf = new mPDF('UTF-8');
    $mpdf->AddPageByArray(array(
      'orientation' => 'P'
      ));

      $output = '';
      $output = '<html><head>';
      $output .= '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>';
      if (@$options['css'] || isset($r->rpt_xml->head->style)) {
        $output .= '<style type="text/css">';
        $output .= $css;
        if (isset($r->rpt_xml->head->style)) {
          $sheet = (string)$r->rpt_xml->head->style;
          $output .= $sheet;
        }
        $output .= '</style>';
      }
      $output .= '<title>' . $r->title . "</title></head><body class='forena-report $link_class'><h1>" . $r->title . '</h1>' . $html;
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
     );

    foreach ($http_headers as $name => $value) {
      $value = preg_replace('/\r?\n(?!\t| )/', '', $value);
      drupal_add_http_header($name, $value);
    }
    return TRUE;
  }
}
