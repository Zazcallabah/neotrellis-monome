
translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
    import(file = "cad-3.5-bracket-br-fill.svg");

    linear_extrude(center=true,height = 10,convexity=10)
    import(file = "cad-3.5-bracket-br-hole.svg");

    translate([158.5,149,2.7])
    linear_extrude(height = 1.5)
    text(text = "O", font = "Consolas", size = 5);
};

translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
    import(file = "cad-3.5-bracket-tr-fill.svg");
    
    linear_extrude(center=true,height = 10,convexity=10)
    import(file = "cad-3.5-bracket-tr-hole.svg");
  
    translate([158.5,188,2.7])
    linear_extrude(height = 1.5)
    text(text = "O", font = "Consolas", size = 5);

    translate([97,278.6,2.7])
    rotate([0,0,90])
    linear_extrude(height = 1.5)
    text(text = "!", font = "Consolas", size = 5);

};




translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
    import(file = "cad-3.5-bracket-bl-fill.svg");

    linear_extrude(center=true,height = 10,convexity=10)
    import(file = "cad-3.5-bracket-bl-hole.svg");

    translate([4.2,94,2.7])
    linear_extrude(height = 1.5)
    text(text = "X", font = "Consolas", size = 5);
};


translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
    import(file = "cad-3.5-bracket-tl-fill.svg");

    linear_extrude(center=true,height = 10,convexity=10)
    import(file = "cad-3.5-bracket-tl-hole.svg");

    translate([3.9,132,2.7])
    linear_extrude(height = 1.5)
    text(text = "X", font = "Consolas", size = 5);

    translate([47,278.6,2.7])
    rotate([0,0,90])
    linear_extrude(height = 1.5)
    text(text = "!", font = "Consolas", size = 5);
};
