<div id="pane-<?php print $entity->id; ?>-tabs">
  <ul>
    <?php foreach ($tabs as $tab): ?>
      <li><a href="#pane-<?php print $entity->id; ?>-tabs-<?php print $tab->index; ?>"><?php print $tab->title; ?></a></li>
    <?php endforeach; ?>
  </ul>

  <?php foreach ($tabs as $tab): ?>
    <div id="pane-<?php print $entity->id; ?>-tabs-<?php print $tab->index; ?>">
      <?php print $tab->content; ?>
    </div>
  <?php endforeach; ?>
</div>
