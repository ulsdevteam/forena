<?php
/**
 * @file FrxSVGGraph
 * php SVG Graph generator
 *
 * @author davidmetzler
 *
 */
require_once ('FrxChart.inc');
class FrxSVGGraph extends FrxChart {
  private $graph;
  private $links;
  function __construct() {
    $directory = FrxReportGenerator::instance()->configuration('library_path');
    $library = rtrim($directory, '/') . '/SVGGraph/SVGGraph.php';
    require_once $library;
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


}