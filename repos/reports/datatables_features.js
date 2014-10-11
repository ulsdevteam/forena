/**
 * @file
 * Set behaviors related to dataTables Features.
 */

(function ($) {
  Drupal.behaviors.SetDatatablesFeatures = {
    attach: function (context, settings) {
      $('.frxTable table').dataTable({
        "sPaginationType": "full_numbers",
        "stateSave": true
      });
    }
  };
  
})(jQuery);


/**
 * More dataTables features to pick from (use a comma to separate them, except for the last one):
        "dom": '<"top"i>rt<"bottom"flp><"clear">',
	"sPaginationType": "two_button"
      }); 
 */