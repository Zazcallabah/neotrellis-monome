
intersection(){
//difference(){
translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10) 
        import(file = "cad-3-three-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10) 
        import(file = "cad-3-three-hole.svg");
};

translate([0,0,-10])union()
{
    cube([70,187,20]);
    cube([187,135,20]);
};
}