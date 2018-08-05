#define EPS
#include "uml/uml.h"

namespace uml
{

eps::lineending_t* lineending_comments()
{ 
    static eps::lineending_circle_center_t lineending_comments("umllineendingcomments", static_cast<float>(3._bp), false);
    return &lineending_comments;
}

eps::lineending_t* lineending_initial_state()
{
    static eps::lineending_circle_center_t lineending_initial_state("umllineendinginitialstate", static_cast<float>(9._bp), true);
    return &lineending_initial_state;
}

static eps::lineending_triangle_arrow_t lineending_open_arrow("umllineendingopenarrow", static_cast<float>(8._bp), static_cast<float>(5._bp), false, false, true);
static eps::lineending_triangle_arrow_t lineending_filled_arrow("umllineendingfilledarrow", static_cast<float>(8._bp), static_cast<float>(5._bp), true, true, false);

eps::lineending_t* lineending_asynchronous_message()
{
    return &lineending_open_arrow;
}

eps::lineending_t* lineending_synchronous_message()
{
    return &lineending_filled_arrow;
}

eps::linestyle_t* linestyle_dash() 
{ 
    static eps::linestyle_symetric_dash_t linestyle_dash(static_cast<float>(2.6_bp));
    return &linestyle_dash;
}


simple_class_t::simple_class_t(
    eps::iproperties_t const& parent_properties,
    std::string const& text)
    : eps::group_t(parent_properties)
    , m_rectangle(*eps::add<eps::rectangle_t>(this, m_default_width, m_default_height))
    , m_text(*eps::add<eps::text_t>(this, text, eps::text_ref_t::cc))
{ 
    m_rectangle.setfillgray(solid_whiteness);
    m_rectangle.fill();
}

eps::point_t simple_class_t::get_ref(eps::ref_t ref)
{
    return uml::get_ref(ref,
        m_rectangle[eps::rectangle_t::tr],
        m_rectangle[eps::rectangle_t::tl],
        m_rectangle[eps::rectangle_t::bl],
        m_rectangle[eps::rectangle_t::br]);
}

sequence_diagram_t::sequence_diagram_t(
    eps::iproperties_t const& parent_properties,
    int lanes)
    : eps::group_t(parent_properties)
    , m_lanes(lanes)
    , m_time(0)
    , m_time_advance(grid_space)
    , m_context_width(grid_space)
    , m_default_lane_space(grid_space * 10)
{
    // begin uml graphics style
    setlinewidth(line_width);
    setlinecap(eps::cap_t::round);
    setlinejoin(eps::join_t::round);
    // end uml graphics style
    float x = 0;
    for (lane_t& lane : m_lanes)
    {
        lane.m_x = x;
        x += m_default_lane_space;
    }
}

void sequence_diagram_t::add_simple_class(int lane, std::string const& text)
{
    m_lanes[lane].m_pclass = eps::add<simple_class_t>(this, text);
    m_lanes[lane].m_pclass->move(eps::point_t(0, 0) - m_lanes[lane].m_pclass->get_ref(eps::ref_t::bc));
    m_lanes[lane].m_pclass->move(eps::vect_t(m_lanes[lane].m_x, m_time));
}

void sequence_diagram_t::start_lifeline(int lane)
{
    m_lanes[lane].m_plifeline = eps::add<eps::line_t>(this, eps::point_t(m_lanes[lane].m_x, m_time), eps::point_t(m_lanes[lane].m_x, m_time));
    m_lanes[lane].m_plifeline->setlinestyle(linestyle_dash());
}


void sequence_diagram_t::lifeline_space(int lane, float space)
{
    if (lane + 1 > static_cast<int>(m_lanes.size()))
    {
        THROW(std::logic_error, "E0201", << "No lane '" << lane + 1 << "'");
    }
    if (lane < 0)
    {
        THROW(std::logic_error, "E0201", << "No lane '" << lane << "'");
    }
    float correction = space - (m_lanes[lane + 1].m_x - m_lanes[lane].m_x);
    for (int i=lane+1; i<static_cast<int>(m_lanes.size()); ++i)
    {
        m_lanes[i].m_x += correction;
    }
}

void sequence_diagram_t::end_lifeline(int lane, bool destroy)
{
    eps::point_t end_point(m_lanes[lane].m_x, m_time);
    (*m_lanes[lane].m_plifeline)[eps::line_t::b] = end_point;
    if (destroy)
    {
        eps::add<eps::line_t>(this, end_point + eps::vect_t(-grid_space / 2, -grid_space / 2), end_point + eps::vect_t(+grid_space / 2, +grid_space / 2));
        eps::add<eps::line_t>(this, end_point + eps::vect_t(-grid_space / 2, +grid_space / 2), end_point + eps::vect_t(+grid_space / 2, -grid_space / 2));
    }
}

void sequence_diagram_t::start_context(int lane)
{
    m_lanes[lane].m_context_stack.push(eps::add<eps::rectangle_t>(this, m_context_width, 0.f));
    m_lanes[lane].m_context_stack.top()->move(eps::point_t(m_lanes[lane].m_x, m_time));
    m_lanes[lane].m_context_stack.top()->setfillgray(solid_whiteness);
    m_lanes[lane].m_context_stack.top()->fill();
}

void sequence_diagram_t::shift_context(int lane, int shift)
{
    m_lanes[lane].m_context_stack.top()->move(eps::point_t(shift * m_context_width/2, m_time));
}

void sequence_diagram_t::end_context(int lane)
{
    std::stack<eps::rectangle_t*>& context_stack = m_lanes[lane].m_context_stack;
    (*context_stack.top())[eps::rectangle_t::bl].m_y = m_time;
    (*context_stack.top())[eps::rectangle_t::br].m_y = m_time;
    context_stack.pop();
}

void sequence_diagram_t::found_async_message(int from, int to, std::string const& event_text)
{
    eps::line_t* pline = eps::add<eps::line_t>(this, eps::point_t(m_lanes[from].m_x, m_time), eps::point_t(m_lanes[to].m_x, m_time));
    pline->setlinebegin(lineending_initial_state());
    pline->setlineend(lineending_asynchronous_message());
    text_above_line(pline, event_text);
}

void sequence_diagram_t::found_sync_message(int from, int to, std::string const& event_text)
{
    if (m_lanes[to].m_context_stack.size() <= 0)
    {
        THROW(std::logic_error, "E0203", << "No context for lane '" << to << "'");
    }
    float x_from;
    float x_to;
    calculate_from_to(from, to, x_from, x_to);
    x_from = m_lanes[from].m_x;
    eps::line_t* pline = eps::add<eps::line_t>(this, eps::point_t(x_from, m_time), eps::point_t(x_to, m_time));
    pline->setlinebegin(lineending_initial_state());
    pline->setlineend(lineending_synchronous_message());
    text_above_line(pline, event_text);
}

void sequence_diagram_t::async_message(int from, int to, std::string const& event_text)
{
    float x_from;
    float x_to;
    calculate_from_to(from, to, x_from, x_to);
    eps::line_t* pline = eps::add<eps::line_t>(this, eps::point_t(x_from, m_time), eps::point_t(x_to, m_time));
    pline->setlineend(lineending_asynchronous_message());
    text_above_line(pline, event_text);
}

void sequence_diagram_t::sync_message(int from, int to, std::string const& event_text)
{
    float x_from;
    float x_to;
    calculate_from_to(from, to, x_from, x_to);
    eps::line_t* pline = eps::add<eps::line_t>(this, eps::point_t(x_from, m_time), eps::point_t(x_to, m_time));
    pline->setlineend(lineending_synchronous_message());
    text_above_line(pline, event_text);
}

void sequence_diagram_t::return_message(int from, int to, std::string const& event_text)
{
    float x_from;
    float x_to;
    calculate_from_to(from, to, x_from, x_to);
    eps::line_t* pline = eps::add<eps::line_t>(this, eps::point_t(x_from, m_time), eps::point_t(x_to, m_time));
    pline->setlineend(lineending_asynchronous_message());
    pline->setlinestyle(linestyle_dash());
    text_above_line(pline, event_text);
}

void sequence_diagram_t::create(int from, int to, std::string const& event_text)
{
    float x_from;
    float x_to;
    calculate_from_to(from, to, x_from, x_to);
    float y_from = m_time + m_time_advance;
    x_to = m_lanes[to].m_x;
    float y_to = 0;
    simple_class_t* pclass = m_lanes[to].m_pclass;
    if (pclass)
    {
        x_to = (from <= to) ? pclass->m_rectangle[eps::rectangle_t::bl].m_x : pclass->m_rectangle[eps::rectangle_t::bl].m_y;
        y_to = pclass->m_rectangle[eps::rectangle_t::bl].m_y + m_time_advance;
    }
    eps::line_t* pline = eps::add<eps::line_t>(this, eps::point_t(x_from, y_from), eps::point_t(x_to, y_to));
    pline->setlineend(lineending_asynchronous_message());
    pline->setlinestyle(linestyle_dash());
    text_above_line(pline, event_text);
}

void sequence_diagram_t::destroy(int from, int to, std::string const& event_text)
{
    float x_from;
    float x_to;
    calculate_from_to(from, to, x_from, x_to);
    eps::line_t* pline = eps::add<eps::line_t>(this, eps::point_t(x_from, m_time), eps::point_t(x_to, m_time));
    pline->setlineend(lineending_asynchronous_message());
    pline->setlinestyle(linestyle_dash());
    text_above_line(pline, event_text);
}

void sequence_diagram_t::calculate_from_to(int from, int to, float& x_from, float& x_to)
{
    x_from = m_lanes[from].m_x;
    std::stack<eps::rectangle_t*>const& from_context_stack = m_lanes[from].m_context_stack;
    if (!from_context_stack.empty())
    {
        x_from = (from <= to) ? (*from_context_stack.top())[eps::rectangle_t::br].m_x : (*from_context_stack.top())[eps::rectangle_t::bl].m_x;
    }
    x_to = m_lanes[to].m_x;
    std::stack<eps::rectangle_t*>const& to_context_stack = m_lanes[to].m_context_stack;
    if (!to_context_stack.empty())
    {
        x_to = (from <= to) ? (*to_context_stack.top())[eps::rectangle_t::bl].m_x : (*to_context_stack.top())[eps::rectangle_t::br].m_x;
    }
}

void sequence_diagram_t::text_above_line(eps::line_t* pline, std::string const& text)
{
    if (!text.empty())
    {
        eps::text_t *ptext = eps::add<eps::text_t>(this, text, eps::text_ref_t::Bc);
        ptext->move(eps::mid((*pline)[eps::line_t::a], (*pline)[eps::line_t::b]));
        ptext->move(eps::vect_t(0,static_cast<float>(1._bp)));
    }
}

}; // namespace uml