<?php
/**
 * @file FrxGraphTemplate.inc
 * Graphing Template,
 * Generates graphs using phpSVG.
 * @author davidmetzler
 *
 */
class FrxGraphTemplate extends FrxTemplate {

  public function config_form($config, $xml='') {
    $form_ctl = array();

    $graph_types = array(
      'bargraph' => 'Bar Graph',
      'piechart' => 'Pie Chart',
      'linegraph' => 'Line Graph',
      'multilinegraph' => 'Multi Series Line Graph',
      'scatterplot' => 'Scatter Plot',
    );

    $styles = array(
      '' => 'Normal',
      '3D' => '3D',
    );

    if ($config['type']=='bargraph') $styles = $styles + array('grouped' => 'Grouped', 'stacked' => 'Stacked', 'horizontal' => 'Horizontal');
    switch (@$config['type']) {
      case 'piechart':
      case 'linegraph':
        $num_series = 1;
        $xvalues = FALSE;
        break;
      case 'scatterplot':
        $num_series = 4;
        $xvalues = TRUE;
        break;
      default:
        $xvalues = FALSE;
        $num_series = 4;
    }

    $form_ctl['type'] = array(
      '#type' => 'select',
      '#title' => t('Graph Type'),
      '#options' => $graph_types,
      '#default_value' => $config['type'],
    );
    forena_template_ajax($form_ctl['type']);

    $form_ctl['gen_table'] = array(
      '#type' => 'checkbox',
      '#title' => t('Include data table with graph'),
      '#default_value' => $config['gen_table'],
    );

   $form_ctl['style'] = array(
      '#type' => 'select',
      '#title' => t('Style'),
      '#options' => $styles,
      '#disabled' => ($config['type'] == 'scatterplot'),
      '#default_value' => $config['style']
    );


    $form_ctl['label'] = array(
      '#type' => 'textfield',
      '#title' => t('Label'),
      '#default_value' => $config['label'],
    );


    $form_ctl['series'] = array(
      '#type' => 'fieldset',
      '#title' => t('Series'),
    );
    if ($xvalues) {
      $form_ctl['seriesx'] = array(
        '#type' => 'fieldset',
        '#title' => t('Series X')
      );
      $form_ctl['series']['#title'] = t('Series Y');
    }


    for ($i=0; $i <= $num_series -1; $i++) {
      if ($xvalues) {
        $form_ctl['seriesx'][$i] = array(
          '#type' => 'textfield',
          '#title' => check_plain((string)($i + 1)),
          '#default_value' => $config['seriesx'][$i]
        );
      }
      $form_ctl['series'][$i] = array(
        '#type' => 'textfield',
        '#title' => check_plain((string)($i + 1)),
        '#default_value' => $config['series'][$i],
        );

    }

    $form_ctl['link'] = array(
      '#type' => 'textfield',
      '#title' => 'link',
      '#default_value' => $config['link'],
    );

    return $form_ctl;
  }

  public function generate($xml, $config) {
    $media = $this->extract('media', $config);
    $media = $media ? $media : 'FrxSVGGraph';
    $div = $this->blockDiv($config);
    if ($media == 'FrxSVGGraph') {
      // Clean series
      //if (count($config['series'])==1) $config['series'] = $config['series'][1];
      //if (count($config['seriesx'])==1) $config['seriesx'] = $config['seriesx'][1];
      foreach ($config['series'] as $i => $series) if (!$series) unset($config['series'][$i]);
      // Clean colors
      if (isset($config['colors'])) foreach ($config['colors'] as $i => $color) if (!$color) unset($color[$i]);
      $type = $this->extract('type', $config);
      $xpath = $this->extract('xpath', $config);
      $clause = $this->extract('clause', $config);
      $gen_table = $this->extract('gen_table', $config);
      $frxparms = array(
        'type' => $type,
        'renderer' => 'FrxSVGGraph',
        'xpath' => $xpath,
        'options' => forena_query_string($config),
      );
      $svg = $this->addNode($div, 2, 'svg', NULL, NULL, $frxparms);
      if ($gen_table) {
        $columns = $this->columns($xml);
        $table = $this->addNode($div, 4, 'table');
        $thead = $this->addNode($table, 6, 'thead');
        $throw = $this->addNode($thead, 8, 'tr');
        $tbody = $this->addNode($table, 6, 'tbody');
        $tdrow = $this->addNode($tbody, 8, 'tr', NULL, NULL, array('foreach' => '*'));
        if ($columns) foreach ($columns as $col) {
           $this->addNode($throw, 10, 'th', $col);
           $this->addNode($tdrow, 10, 'td', '{' . $col . '}');
        }
      }

    }
  }

}