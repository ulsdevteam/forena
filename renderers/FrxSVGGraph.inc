<?php
/**
 * @file FrxSVGGraph
 * php SVG Graph generator
 *
 * @author davidmetzler
 *
 */
class FrxSVGGraph extends FrxRenderer {
  private $graph;
  private $links;
  public $templateName = 'SVG Graph';
  public $xy_data = FALSE;

  public function __construct() {
    parent::__construct();
    $library = forena_library_file('SVGGraph');
    require_once $library;
  }

  public function scrapeConfig() {
    $nodes = $this->reportDocDomNode->getElementsByTagName('svg');
    if ($nodes->length) {
      $svg = $nodes->item(0);
      $svg = simplexml_import_dom($svg);

      $config = $this->mergedAttributes($svg);
      $graph_type = isset($config['type']) ? strtolower($config['type']) : 'bargraph';
      $types = $this->graphTypes();
      $types = array_change_key_case($types);
      $config['type'] = $types[$graph_type]['type'];
      $config['style'] = $types[$graph_type]['style'];
    }
    return $config;
  }

  public function render() {

    // Get data from source
    $output = '';
    $options = array();
    $series = array();
    $seriesx = array();
    $data = array();
    $attributes = $this->mergedAttributes();
    $path = isset($attributes['xpath']) ? $attributes['xpath'] :  '*';
    $label = '';
    $links = '';
    $type = $attributes['type'];
    $link = @$attributes['link'];
    // Legacy options.  New charts should be generated using Frx:attribute syntax
    if (isset($attributes['options'])) {
      parse_str($attributes['options'], $options);
      unset($attributes['options']);
      $options  = array_merge($attributes, $options);
    }
    else {
      $options = $attributes;
    }

    $series = @$options['series'];
    if (isset($options['seriesx']))  {
      $seriesx = $options['seriesx'];
      unset($options['seriesx']);
    }
    if (isset($options['label'])) {
      $label =  $options['label'];
      unset($options['label']);
    }
    foreach ($options as $key => $value) {
      $options[$key] = $this->teng->replace($value, TRUE);
    }

    if (isset($attributes['height'])) $options['height'] = $this->teng->replace($attributes['height']);
    if (isset($attributes['width'])) $options['width'] = $this->teng->replace($attributes['width']);

    // Extract chart series data from xml.
    if ($series) {
      foreach ((array)$series as $i => $d) {
        if ($seriesx) {
          $this->xy_data = TRUE;
          $data[] = $this->xmlToValues($path, $d, $seriesx[$i], TRUE);
        }
        else {
          $data[] = $this->xmlToValues($path, $d, $label, FALSE);
        }
      }
    }

    if ($link) {
      $links = $this->xmlToValues($path, $link, $label);
    }

    // Deal with rare case where attributes are supposed to come from data
    if (@$attributes['color'] && !is_array($attributes['color']) && strpos($attributes['color'], '{') !==FALSE) {
      $options['colors'] = $this->xmlToValues($path, $attributes['color']);
    }

    if ($data) {

      if (method_exists($this, 'renderChart')) {
        $output = $this->renderChart($type, $data, $options, $links);
      }

    }
    return $output;
  }

  static function graphTypes() {
    return array(
      'BarGraph' => array('type' => 'Bar Graph', 'style' => 'Simple', 'series' => 1),
      'Bar3DGraph' => array('type' => 'Bar Graph', 'style' => '3D', 'series' => 1),
      'StackedBarGraph' => array('type' => 'Bar Graph', 'style' => 'Stacked', 'series' => 4),
      'GroupedBarGraph' => array('type' => 'Bar Graph', 'style' => 'Grouped', 'series' => 4),
      'CylinderGraph' => array('type' => 'Bar Graph', 'style' => 'Cylinder', 'series' => 1),
      'StackedCylinderGraph' => array('type' => 'Bar Graph', 'style' => 'Stacked Cylinder', 'series' => 4),
      'GroupedCylinderGraph' =>  array('type' => 'Bar Graph', 'style' => 'Grouped Cylinder', 'series' => 1),
      'PieGraph' => array('type' => 'Pie Chart', 'style' => 'Simple', 'series' => 1),
      'Pie3DGraph' => array('type' => 'Pie Chart', 'style' => '3D', 'series' => 1),
      'HorizontalBarGraph' => array('type' => 'Bar Graph', 'style' => 'Horizontal', 'series' => 1),
      'LineGraph' =>array('type' => 'Line Graph', 'style' => 'Simple', 'series' => 1),
      'MultiLineGraph' => array('type' => 'Line Graph', 'style' => 'Multi', 'series' => 4),
      'ScatterGraph' => array('type' => 'Scatter Plot', 'style' => 'Simple', 'series' => 1, 'xaxis' => TRUE),
      'MultiScatterGraph' => array('type' => 'Scatter Plot', 'style' => '3D', 'series' => 4, 'xaxis' => TRUE),
      'RadarGraph' => array('type' => 'Radar Graph', 'style' => 'Simple', 'series' => 1),
      'MultiRadarGraph' => array('type' => 'Radar Graph', 'style' => 'Multi', 'series' => 4),
    );
  }

  static function graphOptions() {
    $data = FrxSVGGraph::graphTypes();
    foreach($data as $key => $value) {
      $type[$value['type']] = $value['type'];
      $style[$value['type']][$key] = $value['style'];
    }
    return array('types' => $type, 'styles' => $style);
  }

  function renderChart($type, $data, $options, $links ) {
    $options['width'] = @$options['width'] ? $options['width'] : 600;
    $options['height'] = @$options['height'] ? $options['height'] : 400;
    $this->links = $links;
    $type = strtolower($type);
    // Legacy sustitions for backcward compatibility.
    if ($type == 'piechart') $type = 'piegraph';
    if ($type == 'scatterplot') $type = 'scattergraph';
    if ($type == 'multiscatterplot') $type = 'multiscattergraph';

    // Newly defined types
    $graph_types = FrxSVGGraph::graphTypes();

    // Build map for array types.
    $lower_graphs_types = array_change_key_case($graph_types);
    $graph_classes = array_combine(array_keys($lower_graphs_types), array_keys($graph_types));


    if ($data && isset($graph_classes[$type])) {
      $class = $graph_classes[$type];
      if (@$graph_types[$class]['series'] == 1) {
        $output = $this->renderGraph($class, $options, $data[0]);
      }
      else {
        $output = $this->renderGraph($class, $options, $data);
      }
    }
    return $output;
  }


  function renderGraph($type, $options, $data) {
    static $jsinc = FALSE;
    if (!isset($options['scatter_2d']) && ($type == 'ScatterGraph' || $type=='MultiScatterGraph') && $this->xy_data && !isset($options['scatter_2d'])) {
      $options['scatter_2d'] = TRUE;
    }
    else {
      $options['scatter_2d'] = (bool) @$options['scatter_2d'];
    }
    $links = $this->links;
    $width = (@$options['width']) ? @$options['width'] : 600;
    $height = (@$options['height']) ? @$options['height'] : 400;
    $graph = new SVGGraph($width, $height, $options);
    $this->graph = $graph;
    $graph->Values($data);
    if (is_array(@$options['colors'])) {
      $graph->colours = $options['colors'];
    }

    if ($links) {
      $graph->Links($links);
    }
    $output = $graph->Fetch($type, FALSE);
    // Add a viewbox to be compatible with Prince PDF generation.
    $output = str_replace('<svg width', "<svg viewBox='0 0 $width $height' width", $output);
    $javascript = '/<script[^>]*?>.*?<\/script>/si';
    $noscript = '';
    if (!$jsinc && $this->format == 'web') {
      $output .= $graph->FetchJavascript();
    }
    else {
      $output = preg_replace($javascript, $noscript, $output);
    }

    return $output;
  }

  public function configForm($config, $xml='') {
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
    $form_ctl['xpath'] = array(
      '#type' => 'textfield',
      '#title' => ('xpath'),
      '#default_value' => @$config['xpath'],
    );

    $form_ctl['type'] = array(
        '#type' => 'select',
        '#title' => t('Graph Type'),
        '#options' => $graph_options['types'],
        '#default_value' => $type,
        '#ajax' => $this->configAjax(),
    );

    $form_ctl['style'] = array(
        '#type' => 'select',
        '#title' => t('Style'),
        '#options' => $styles,
        '#default_value' => $style,
        '#ajax' => $this->configAjax(),
    );

    $form_ctl['gen_table'] = array(
        '#type' => 'checkbox',
        '#title' => t('Include data table with graph'),
        '#default_value' => @$config['gen_table'],
        '#ajax' => $this->configAjax(),
    );

    $form_ctl['label'] = array(
        '#type' => 'textfield',
        '#title' => t('Label'),
        '#default_value' => @$config['label'],
        '#ajax' => $this->configAjax('blur'),
    );


    $form_ctl['series'] = array(
        '#type' => 'fieldset',
        '#title' => t('Series'),
        '#ajax' => $this->configAjax('blur'),
    );
    if ($xvalues) {
      $form_ctl['seriesx'] = array(
          '#type' => 'fieldset',
          '#title' => t('Series X'),
          '#description' => t('Forena token expression to get the X value (.e.g. {years}'),
          '#ajax' => $this->configAjax('blur'),
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
    $config['class'] = get_class($this);
    $media = 'FrxSVGGraph';
    $div = $this->blockDiv($config);
    if ($config) {
      // Clean series
      foreach ($config['series'] as $i => $series) if (!$series) unset($config['series'][$i]);
      // Clean colors
      if (isset($config['colors'])) foreach ($config['colors'] as $i => $color) if (!$color) unset($color[$i]);

      $type = $this->extract('type', $config);
      if (!$type) $type = 'Bar Graph';

      $gen_table = $this->extract('gen_table', $config);
      $style = $this->extract('style', $config);
      if (!$style) $style = 'BarGraph';
      $graph_options = FrxSVGGraph::graphOptions();
      if (!isset($graph_options['styles'][$type][$style])) {
        $styles = array_keys($graph_options['styles'][$type]);
        $style = $styles[0];
      }
      if ($style) $type = $style;
      $frxattrs = $this->arrayAttributes($config);
      $frxattrs['renderer'] = 'FrxSVGGraph';
      $frxattrs['type'] = $type;


      $svg = $this->setFirstNode($div, 2, 'svg',  NULL, NULL,  $frxattrs);
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