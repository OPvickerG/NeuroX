"""STL meshes -> STEP solids (faceted B-rep, coplanar triangles merged) for Fusion 360 / any CAD.
usage: stl2step.py out.step part.stl [part2.stl ...]            one solid per file (named after the file)
       stl2step.py --asm out.step name:colourhex:file.stl ...    coloured assembly (XCAF)"""
import sys, os
from OCP.StlAPI import StlAPI_Reader
from OCP.TopoDS import TopoDS_Shape, TopoDS
from OCP.BRepBuilderAPI import BRepBuilderAPI_Sewing, BRepBuilderAPI_MakeSolid
from OCP.ShapeUpgrade import ShapeUpgrade_UnifySameDomain
from OCP.ShapeFix import ShapeFix_Solid
from OCP.TopAbs import TopAbs_SHELL, TopAbs_FACE
from OCP.TopExp import TopExp_Explorer
from OCP.STEPControl import STEPControl_Writer, STEPControl_AsIs
from OCP.Interface import Interface_Static
from OCP.IFSelect import IFSelect_RetDone
from OCP.TCollection import TCollection_ExtendedString, TCollection_HAsciiString
from OCP.TDocStd import TDocStd_Document
from OCP.XCAFDoc import XCAFDoc_DocumentTool, XCAFDoc_ColorType
from OCP.XCAFApp import XCAFApp_Application
from OCP.STEPCAFControl import STEPCAFControl_Writer
from OCP.Quantity import Quantity_Color, Quantity_TOC_RGB
from OCP.TDataStd import TDataStd_Name
from OCP.BRep import BRep_Builder
from OCP.TopoDS import TopoDS_Compound

def solid_from_stl(path):
    shp = TopoDS_Shape(); StlAPI_Reader().Read(shp, path)
    sew = BRepBuilderAPI_Sewing(1e-4); sew.Add(shp); sew.Perform(); sewed = sew.SewedShape()
    ms = BRepBuilderAPI_MakeSolid(); ex = TopExp_Explorer(sewed, TopAbs_SHELL); n = 0
    while ex.More(): ms.Add(TopoDS.Shell_s(ex.Current())); ex.Next(); n += 1
    solid = ms.Solid() if n else sewed
    fix = ShapeFix_Solid(solid); fix.Perform(); solid = fix.Solid()
    u = ShapeUpgrade_UnifySameDomain(solid, True, True, False); u.Build()
    return u.Shape()
def nfaces(s):
    ex = TopExp_Explorer(s, TopAbs_FACE); k = 0
    while ex.More(): k += 1; ex.Next()
    return k
def setup():
    Interface_Static.SetCVal_s("write.step.schema", "AP214")
    Interface_Static.SetCVal_s("write.step.unit", "MM")
    Interface_Static.SetCVal_s("write.step.product.name", "NeuroX HALO v2")

if __name__ == "__main__":
    setup()
    if sys.argv[1] == "--asm":
        out = sys.argv[2]
        app = XCAFApp_Application.GetApplication_s(); doc = TDocStd_Document(TCollection_ExtendedString("MDTV-XCAF"))
        app.InitDocument(doc)
        st = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main()); ct = XCAFDoc_DocumentTool.ColorTool_s(doc.Main())
        b = BRep_Builder(); comp = TopoDS_Compound(); b.MakeCompound(comp)
        top = st.NewShape(); TDataStd_Name.Set_s(top, TCollection_ExtendedString("NeuroX_HALO_v2"))
        for arg in sys.argv[3:]:
            name, hexc, f = arg.split(":", 2)
            s = solid_from_stl(f)
            lab = st.AddShape(s, False)
            TDataStd_Name.Set_s(lab, TCollection_ExtendedString(name))
            r, g, bb = (int(hexc[i:i + 2], 16) / 255 for i in (0, 2, 4))
            ct.SetColor(lab, Quantity_Color(r, g, bb, Quantity_TOC_RGB), XCAFDoc_ColorType.XCAFDoc_ColorSurf)
            st.AddComponent(top, lab, s.Location())
            print(f"  {name:28s} {nfaces(s):6d} faces")
        st.UpdateAssemblies()
        w = STEPCAFControl_Writer(); w.SetColorMode(True); w.SetNameMode(True)
        w.Transfer(doc, STEPControl_AsIs); assert w.Write(out) == IFSelect_RetDone
    else:
        out = sys.argv[1]; w = STEPControl_Writer()
        for f in sys.argv[2:]:
            s = solid_from_stl(f); w.Transfer(s, STEPControl_AsIs); print(f"  {os.path.basename(f):28s} {nfaces(s):6d} faces")
        assert w.Write(out) == IFSelect_RetDone
    print(out, os.path.getsize(out) // 1024, "KB")
