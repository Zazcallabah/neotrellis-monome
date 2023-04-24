
//intersection(){
difference(){
translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10) 
        import(file = "cad-4-bottom-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10) 
        import(file = "cad-4-bottom-hole.svg");
};

translate([0,0,-10])union()
{
    cube([90,115,20]);
    translate([80,0,0])
    cube([187,140,20]);
};
}