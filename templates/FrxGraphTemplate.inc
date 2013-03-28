<?php
/**
 * @file FrxGraphTemplate.inc
 * Graphing Template,
 * Generates graphs using phpSVG.
 * @author davidmetzler
 *
 */
require_once (drupal_get_path('module','forena') . '/renderers/FrxSVGGraph.inc');
class FrxGraphTemplate extends FrxTemplate {

  public function config_form($config, $xml='') {
    $form_ctl = array();

    $graph_types = array(
      'bargraph' => 'Bar Graph',
      'linegraph' => 'Line Graph',
      'multilinegraph' => 'Line Graph (Multi Series)',
      'piechart' => 'Pie Chart',
      'radargraph' => 'Radar Graph',
      'multiradargraph' => 'Radar Graph (Multi Series)',
      'scatterplot' => 'Scatter Plot',
    );

    $type = isset($config['type']) ? $config['type'] : 'Bar Graph';
    $style = isset($config['style']) ? $config['style'] : 'BarGraph';
    $graph_options = FrxSVGGraph::graphOptions();
    $graph_types = FrxSVGGraph::graphTypes();
    $styles = $graph_options['styles'][$type];
    $xvalues = @$graph_types[$type]['xaxis'];
    $num_series = isset($graph_types[$style]['series']) ? $graph_types[$style]['series'] : 1;
    $types = $graph_options['types'];

    $form_ctl['type'] = array(
      '#type' => 'select',
      '#title' => t('Graph Type'),
      '#options' => $graph_options['types'],
      '#default_value' => $type,
    );

    forena_template_ajax($form_ctl['type']);
    $form_ctl['style'] = array(
      '#type' => 'select',
      '#title' => t('Style'),
      '#options' => $styles,
      '#default_value' => $style,
    );
    forena_template_ajax($form_ctl['style']);

    $form_ctl['gen_table'] = array(
      '#type' => 'checkbox',
      '#title' => t('Include data table with graph'),
      '#default_value' => @$config['gen_table'],
    );

    $form_ctl['label'] = array(
      '#type' => 'textfield',
      '#title' => t('Label'),
      '#default_value' => @$config['label'],
    );


    $form_ctl['series'] = array(
      '#type' => 'fieldset',
      '#title' => t('Series'),
    );
    if ($xvalues) {
      $form_ctl['seriesx'] = array(
        '#type' => 'fieldset',
        '#title' => t('Series X'),
        '#description' => t('Forena token expression to get the X value (.e.g. {years}'),
      );
      $form_ctl['series']['#title'] = t('Series Y');
    }


    for ($i=0; $i <= $num_series -1; $i++) {
      if ($xvalues) {
        $form_ctl['seriesx'][$i] = array(
          '#type' => 'textfield',
          '#title' => check_plain((string)($i + 1)),
          '#default_value' => $config['seriesx'][$i],
          '#description' => t('Forena token expressioin for series %s x value. (e.g. {total} )', array('%s' => $i+1)),
        );
      }
      $form_ctl['series'][$i] = array(
        '#type' => 'textfield',
        '#title' => check_plain((string)($i + 1)),
        '#default_value' => @$config['series'][$i],
        '#description' => t('Forena token expressioin for series %s. (e.g. {total} )', array('%s' => $i+1)),
      );

    }

    $form_ctl['link'] = array(
      '#type' => 'textfield',
      '#title' => 'link',
      '#default_value' => @$config['link'],
    );

    return $form_ctl;
  }

  public function generate($xml, $config) {
    $media = $this->extract('media', $config);
    $media = $media ? $media : 'FrxSVGGraph';
    $div = $this->blockDiv($config);
    if ($media == 'FrxSVGGraph' && $config) {
      // Clean series
      //if (count($config['series'])==1) $config['series'] = $config['series'][1];
      //if (count($config['seriesx'])==1) $config['seriesx'] = $config['seriesx'][1];
      foreach ($config['series'] as $i => $series) if (!$series) unset($config['series'][$i]);
      // Clean colors
      if (isset($config['colors'])) foreach ($config['colors'] as $i => $color) if (!$color) unset($color[$i]);
      $type = $this->extract('type', $config);
      if (!$type) $type = 'Bar Graph';

      $xpath = $this->extract('xpath', $config);
      $clause = $this->extract('clause', $config);
      $gen_table = $this->extract('gen_table', $config);
      $style = $this->extract('style', $config);
      if (!$style) $style = 'BarGraph';
      $graph_options = FrxSVGGraph::graphOptions();
      if (!isset($graph_options['styles'][$type][$style])) {
        $styles = array_keys($graph_options['styles'][$type]);
        $style = $styles[0];
      }
      if ($style) $type = $style;

      $frxparms = array(
        'type' => $type,
        'renderer' => 'FrxSVGGraph',
        'xpath' => $xpath,
      );
      $frxparms = array_merge($config, $frxparms);
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