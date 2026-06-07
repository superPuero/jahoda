#include "galaxy_nn.h"

static f64 parse_f64(arena *static_arena, arena *temp_arena, str *text, u64 *current_char)
{
    u64 start = *current_char;
    char c = text->data[*current_char];

    while(isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e')
    {
        (*current_char)++;
        c = text->data[*current_char];
    }

    (*current_char)++;

    char *end = NULL;
    str ntstr = str_from_view_nt(temp_arena, (strv){.data = text->data + start, *current_char - start - 1});

    return strtod(ntstr.data, &end);
}

static celestial_type parse_celestial_type(arena *static_arena, arena *temp_arena, str *text, u64 *current_char)
{
    (*current_char)++;

    u64 start = *current_char;

    char c = text->data[*current_char];

    while(isalpha(c))
    {
        (*current_char)++;
        c = text->data[*current_char];
    }

    
    str ntstr = str_from_view_nt(temp_arena, (strv){.data = text->data + start, *current_char - start});

    (*current_char)++;
    (*current_char)++;

    if(strncmp(ntstr.data, "QSO", strlen("QSO")) == 0)
    {
        return celestial_type_quasar;
    }    
    else if(strncmp(ntstr.data, "STAR", strlen("STAR")) == 0)
    {
        return celestial_type_star;
    }    
    else if(strncmp(ntstr.data, "GALAXY", strlen("GALAXY")) == 0)
    {
        return celestial_type_galaxy;
    }
}

f64 score(galaxy_dataset *df, galaxy_nn_model *model, celestial_type type, uz entry)
{
    f64 out = model->biases[type];

    #define XX(feat_id, feat) out += model->weights[type][feat_id] * df->celestials.data[entry].feat;
    celestial_features_use
    #undef XX

    return out;
}

f64 softmax(galaxy_dataset *df, galaxy_nn_model *model, celestial_type type, uz entry)
{    
    f64 sum = 0.0f;

    for(celestial_type i = 0; i < celestial_type_count; i++)
    {
        sum += exp(score(df, model, i, entry));
    }

    return exp(score(df, model, type, entry)) / sum;
}

f64 cce_loss_gradient(galaxy_dataset *df, galaxy_nn_model *model, celestial_type type, uz entry)
{
    f64 out = 0.0;

    return softmax(df, model, type, entry) - (df->celestials.data[entry].class == type ? 1 : 0);
}

void galaxy_nn_train_round(galaxy_dataset *df, galaxy_nn_model *model)
{
    uz i = 0;
    da_foreach(&df->celestials)
    {
        f64 star_grad = cce_loss_gradient(df, model, celestial_type_star, i);
        f64 quasar_grad = cce_loss_gradient(df, model, celestial_type_quasar, i);
        f64 galaxy_grad = cce_loss_gradient(df, model, celestial_type_galaxy, i);

        model->biases[celestial_type_star] = model->biases[celestial_type_star] - star_grad * model->learning_rate;
        #define XX(feat_id, feat) model->weights[celestial_type_star][feat_id] = model->weights[celestial_type_star][feat_id] - (star_grad * model->learning_rate * df->celestials.data[i].feat);
        celestial_features_use
        #undef XX

        model->biases[celestial_type_quasar] = model->biases[celestial_type_quasar] - quasar_grad * model->learning_rate;
        #define XX(feat_id, feat) model->weights[celestial_type_quasar][feat_id] = model->weights[celestial_type_quasar][feat_id] - (quasar_grad * model->learning_rate * df->celestials.data[i].feat);
        celestial_features_use
        #undef XX

        model->biases[celestial_type_galaxy] = model->biases[celestial_type_galaxy] - galaxy_grad * model->learning_rate;
        #define XX(feat_id, feat) model->weights[celestial_type_galaxy][feat_id] = model->weights[celestial_type_galaxy][feat_id] - (galaxy_grad * model->learning_rate * df->celestials.data[i].feat);
        celestial_features_use
        #undef XX

        i++;
    }
}

galaxy_dataset galaxy_dataset_from_csv(arena *static_arena, arena *temp_arena, strv csv_path)
{
    galaxy_dataset out = {0};
    da_reserve(static_arena, &out.celestials, 54000);
    
    celestial_entry *current_celestial = NULL;
    
    str raw_csv = file_load(temp_arena, csv_path);
    
    u64 current_char = 145;

    while(current_char < raw_csv.occupied - 1)
    {
        if(raw_csv.data[current_char] == '\n')
        {
            current_char++;
        }
        da_append(NULL, &out.celestials, (celestial_entry){0});
        current_celestial = da_last(&out.celestials);

        marker m;
        #define XX(type, feat) m = arena_mark(temp_arena); current_celestial->feat = parse_##type(static_arena, temp_arena, &raw_csv, &current_char); arena_pop_to_marker(m);
        celestial_features
        #undef XX
    }

    return out;
}